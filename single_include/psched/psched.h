
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
  // wait_time() + computation_time()
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
  std::function<void(TaskStats)> task_end;

  // Called if `task_main()` throws an exception
  std::function<void(TaskStats, const char *)> task_error;
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
  // wait_time() + computation_time()
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
  std::atomic_bool done_{false};

  template <class threads, class priority_levels>
  friend class PriorityScheduler;

protected:
  void save_arrival_time() { stats_.arrival_time = std::chrono::steady_clock::now(); }

public:
  Task() {}

  Task(const Task &other) {
    functions_ = other.functions_;
    stats_ = other.stats_;
  }

  Task &operator=(Task other) {
    std::swap(functions_, other.functions_);
    std::swap(stats_, other.stats_);
    return *this;
  }

  template <typename Function> void on_execute(Function &&fn) {
    functions_.task_main = std::forward<Function>(fn);
  }

  template <typename Function> void on_complete(Function &&fn) {
    functions_.task_end = std::forward<Function>(fn);
  }

  template <typename Function> void on_error(Function &&fn) {
    functions_.task_error = std::forward<Function>(fn);
  }

  void operator()() {
    stats_.start_time = std::chrono::steady_clock::now();
    try {
      if (functions_.task_main) {
        functions_.task_main();
      }
      stats_.end_time = std::chrono::steady_clock::now();
      done_ = true;
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

  bool is_done() const { return done_; }
};

} // namespace psched#pragma once
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

}
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
        for (size_t i = 0; i < priority_levels::value; i++) {
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
  ~PriorityScheduler() {
    for (auto &q: priority_queues_)
      q.done();
    for (auto &t : threads_)
      if (t.joinable())
        t.join();
  }

  void schedule(Task &task, size_t priority) {
    if (priority >= priority_levels::value) {
      throw std::runtime_error("Error: Priority " + std::to_string(priority) +
                               " is out of range. Priority should be in range [0, " +
                               std::to_string(priority_levels::value - 1) + "]");
    }

    // Save task arrival time
    task.save_arrival_time();

    // Enqueue task
    while (running_) {
      if (priority_queues_[priority].try_push(task)) {
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

  void start() {
    running_ = true;
    for (unsigned n = 0; n != threads::value; ++n) {
      threads_.emplace_back([this] { run(); });
    }
  }

  void stop() {
    running_ = false;
    ready_.notify_all();
    for (auto &q: priority_queues_)
      q.done();
    for (auto &t : threads_)
      if (t.joinable())
        t.join();
  }
};

} // namespace psched
