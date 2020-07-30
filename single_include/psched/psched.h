
#pragma once
#include <functional>
// #include <psched/task_stats.h>

#include <chrono>

namespace psched {

struct TaskStats {
  using TimePoint = std::chrono::steady_clock::time_point;
  TimePoint arrival_time; // time point when the task is marked as 'ready' (queued)
  TimePoint start_time;   // time point when the task is about to execute (dequeued)
  TimePoint end_time;     // time point when the task completes execution

  // Waiting time is the amount of time spent by a task waiting
  // in the ready queue for getting the CPU.
  template <typename T = std::chrono::milliseconds> long long waiting_time() const {
    return std::chrono::duration_cast<T>(start_time - arrival_time).count();
  }

  // Burst time is the amount of time required by a task for executing on CPU.
  // It is also called as execution time or running time.
  template <typename T = std::chrono::milliseconds> long long burst_time() const {
    return std::chrono::duration_cast<T>(end_time - start_time).count();
  }

  // Turnaround time (TAT) is the time interval from the time of submission
  // of a task to the time of the completion of the task. It can also be
  // considered as the sum of the time periods spent waiting to get into memory or
  // ready queue, execution on CPU and executing input/output.
  //
  // waiting_time() + burst_time()
  template <typename T = std::chrono::milliseconds> long long turnaround_time() const {
    return std::chrono::duration_cast<T>(end_time - arrival_time).count();
  }
};

} // namespace psched

namespace psched {

struct TaskFunctions {
  // Called when the task is (finally) executed by an executor thread
  std::function<void()> task_main;

  // Called after the task has completed executing.
  // In case of exception, `task_error` is called first
  //
  // TaskStats argument can be used to get task computation_time
  // and task response_time.
  std::function<void(const TaskStats &)> task_end;

  // Called if `task_main()` throws an exception
  std::function<void(const TaskStats &, const char *)> task_error;

  TaskFunctions(const std::function<void()> &task_on_execute = {}, 
                const std::function<void(const TaskStats &)> &task_on_complete = {},
                const std::function<void(const TaskStats &, const char *)> &task_on_error = {})
    : task_main(task_on_execute),
      task_end(task_on_complete),
      task_error(task_on_error) {}
};

} // namespace psched
#pragma once
#include <chrono>

namespace psched {

struct TaskStats {
  using TimePoint = std::chrono::steady_clock::time_point;
  TimePoint arrival_time; // time point when the task is marked as 'ready' (queued)
  TimePoint start_time;   // time point when the task is about to execute (dequeued)
  TimePoint end_time;     // time point when the task completes execution

  // Waiting time is the amount of time spent by a task waiting
  // in the ready queue for getting the CPU.
  template <typename T = std::chrono::milliseconds> long long waiting_time() const {
    return std::chrono::duration_cast<T>(start_time - arrival_time).count();
  }

  // Burst time is the amount of time required by a task for executing on CPU.
  // It is also called as execution time or running time.
  template <typename T = std::chrono::milliseconds> long long burst_time() const {
    return std::chrono::duration_cast<T>(end_time - start_time).count();
  }

  // Turnaround time (TAT) is the time interval from the time of submission
  // of a task to the time of the completion of the task. It can also be
  // considered as the sum of the time periods spent waiting to get into memory or
  // ready queue, execution on CPU and executing input/output.
  //
  // waiting_time() + burst_time()
  template <typename T = std::chrono::milliseconds> long long turnaround_time() const {
    return std::chrono::duration_cast<T>(end_time - arrival_time).count();
  }
};

} // namespace psched
#pragma once
#include <atomic>
#include <functional>
// #include <psched/task_functions.h>

namespace psched {

class Task {
  TaskFunctions functions_;
  TaskStats stats_;

  template <class threads, class priority_levels> friend class PriorityScheduler;

protected:
  void save_arrival_time() { stats_.arrival_time = std::chrono::steady_clock::now(); }

public:
  Task(const std::function<void()> &task_on_execute = {}, 
       const std::function<void(const TaskStats &)> &task_on_complete = {},
       const std::function<void(const TaskStats &, const char *)> &task_on_error = {}) 
    : functions_(task_on_execute, task_on_complete, task_on_error) {}

  Task(const Task &other) {
    functions_ = other.functions_;
    stats_ = other.stats_;
  }

  Task &operator=(Task other) {
    std::swap(functions_, other.functions_);
    std::swap(stats_, other.stats_);
    return *this;
  }

  void on_execute(const std::function<void()> &fn) {
    functions_.task_main = fn;
  }

  void on_complete(const std::function<void(const TaskStats &)> &fn) {
    functions_.task_end = fn;
  }

  void on_error(const std::function<void(const TaskStats &, const char *)> &fn) {
    functions_.task_error = fn;
  }

  void operator()() {
    stats_.start_time = std::chrono::steady_clock::now();
    try {
      if (functions_.task_main) {
        functions_.task_main();
      }
      stats_.end_time = std::chrono::steady_clock::now();
    } catch (std::exception &e) {
      stats_.end_time = std::chrono::steady_clock::now();
      if (functions_.task_error) {
        functions_.task_error(stats_, e.what());
      }
    } catch (...) {
      stats_.end_time = std::chrono::steady_clock::now();
      if (functions_.task_error) {
        functions_.task_error(stats_, "Unknown exception");
      }
    }
    if (functions_.task_end) {
      functions_.task_end(stats_);
    }
  }
};

} // namespace psched
#pragma once
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
// #include <psched/task.h>

namespace psched {

class TaskQueue {
  std::deque<Task> queue_;
  bool done_{false};
  std::mutex mutex_;
  std::condition_variable ready_;

public:
  bool try_pop(Task &task) {
    std::unique_lock<std::mutex> lock{mutex_, std::try_to_lock};
    if (!lock || queue_.empty())
      return false;
    task = std::move(queue_.front());
    queue_.pop_front();
    return true;
  }

  template <typename F> bool try_push(F &&f) {
    {
      std::unique_lock<std::mutex> lock{mutex_, std::try_to_lock};
      if (!lock)
        return false;
      queue_.emplace_back(std::forward<F>(f));
    }
    ready_.notify_one();
    return true;
  }

  void done() {
    {
      std::unique_lock<std::mutex> lock{mutex_};
      done_ = true;
    }
    ready_.notify_all();
  }
};

} // namespace psched
#pragma once
#include <array>
#include <atomic>
#include <condition_variable>
#include <mutex>
// #include <psched/task.h>
// #include <psched/task_queue.h>
#include <thread>
#include <vector>

namespace psched {

template <size_t T> struct threads { constexpr static size_t value = T; };

template <size_t P> struct priority_levels { constexpr static size_t value = P; };

template <size_t P> struct priority { constexpr static size_t value = P; };

template <class D, size_t P> struct period { constexpr static D value = D(P); };

template <class threads, class priority_levels> class PriorityScheduler {
  std::vector<std::thread> threads_;
  std::array<TaskQueue, priority_levels::value> priority_queues_;
  std::atomic_bool running_{false};
  std::mutex mutex_;
  std::condition_variable ready_;
  std::atomic_bool enqueued_{false};

  void run() {
    while (running_) {
      // Wait for the `enqueued` signal
      {
        std::unique_lock<std::mutex> lock{mutex_};
        ready_.wait(lock, [this] { return enqueued_.load() || !running_; });
        enqueued_ = false;
      }

      if (!running_) {
        break;
      }

      Task task;
      bool dequeued = false;

      // Start from highest priority queue
      do {
        for (size_t i = priority_levels::value - 1; i >= 0; i--) {
          // Try to pop an item
          if (priority_queues_[i].try_pop(task)) {
            dequeued = true;
            break;
          }
        }
      } while (!dequeued && running_);

      // execute task
      task();
    }
  }

public:
  PriorityScheduler() {
    running_ = true;
    for (unsigned n = 0; n != threads::value; ++n) {
      threads_.emplace_back([this] { run(); });
    }
  }

  ~PriorityScheduler() {
    for (auto &q : priority_queues_)
      q.done();
    for (auto &t : threads_)
      if (t.joinable())
        t.join();
  }

  template <class priority>
  void schedule(Task &task) {
    static_assert(priority::value <= priority_levels::value, "priority out of range");

    // Save task arrival time
    task.save_arrival_time();

    // Enqueue task
    while (running_) {
      if (priority_queues_[priority::value].try_push(task)) {
        break;
      }
    }

    // Send `enqueued` signal to worker threads
    {
      std::unique_lock<std::mutex> lock{mutex_};
      enqueued_ = true;
      ready_.notify_one();
    }
  }

  template <class priority, class period>
  void schedule(Task task) {
    std::thread([this, &task]() {
      do {
        // schedule task at priority level 2
        schedule<priority>(task);

        // sleep for 100ms
        std::this_thread::sleep_for(period::value);
      } while (true);
    }).detach();
  }

  void stop() {
    running_ = false;
    ready_.notify_all();
    for (auto &q : priority_queues_)
      q.done();
    for (auto &t : threads_)
      if (t.joinable())
        t.join();
  }
};

} // namespace psched
