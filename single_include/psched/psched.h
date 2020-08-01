
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
// #include <psched/task_stats.h>

namespace psched {

class Task {
  // Called when the task is (finally) executed by an executor thread
  std::function<void()> task_main_;

  // Called after the task has completed executing.
  // In case of exception, `task_error` is called first
  //
  // TaskStats argument can be used to get task computation_time
  // and task response_time.
  std::function<void(const TaskStats &)> task_end_;

  // Called if `task_main()` throws an exception
  std::function<void(const char *)> task_error_;

  // Temporal behavior of Task
  // Stats includes arrival_time, start_time, end_time
  // Stats can be used to calculate waiting_time, burst_time, turnaround_time
  TaskStats stats_;

  template <class enforce_queue_size> friend class TaskQueue;

protected:
  void save_arrival_time() { stats_.arrival_time = std::chrono::steady_clock::now(); }

public:
  Task(const std::function<void()> &task_main = {},
       const std::function<void(const TaskStats &)> &task_end = {},
       const std::function<void(const char *)> &task_error = {})
      : task_main_(task_main), task_end_(task_end), task_error_(task_error) {}

  Task(const Task &other) {
    task_main_ = other.task_main_;
    task_end_ = other.task_end_;
    task_error_ = other.task_error_;
    stats_ = other.stats_;
  }

  Task &operator=(Task other) {
    std::swap(task_main_, other.task_main_);
    std::swap(task_end_, other.task_end_);
    std::swap(task_error_, other.task_error_);
    std::swap(stats_, other.stats_);
    return *this;
  }

  void on_execute(const std::function<void()> &fn) { task_main_ = fn; }

  void on_complete(const std::function<void(const TaskStats &)> &fn) { task_end_ = fn; }

  void on_error(const std::function<void(const char *)> &fn) { task_error_ = fn; }

  void operator()() {
    stats_.start_time = std::chrono::steady_clock::now();
    try {
      if (task_main_) {
        task_main_();
      }
      stats_.end_time = std::chrono::steady_clock::now();
    } catch (std::exception &e) {
      stats_.end_time = std::chrono::steady_clock::now();
      if (task_error_) {
        task_error_(e.what());
      }
    } catch (...) {
      stats_.end_time = std::chrono::steady_clock::now();
      if (task_error_) {
        task_error_("Unknown exception");
      }
    }
    if (task_end_) {
      task_end_(stats_);
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

enum class remove_task { oldest, newest };

template <size_t size = 0, remove_task policy = remove_task::oldest> struct maintain_queue_size {
  constexpr static bool is_bounded = (size > 0);
  constexpr static size_t value = size;
  constexpr static remove_task remove_policy = policy;
};

template <class queue_size = maintain_queue_size<>> class TaskQueue {
  std::deque<Task> queue_;        // Internal queue data structure
  bool done_{false};              // Set to true when no more tasks are expected
  std::mutex mutex_;              // Mutex for the internal queue
  std::condition_variable ready_; // Signal for when a task is enqueued

public:
  bool try_pop(Task &task) {
    std::unique_lock<std::mutex> lock{mutex_, std::try_to_lock};
    if (!lock || queue_.empty())
      return false;
    task = std::move(queue_.front());
    queue_.pop_front();
    return true;
  }

  bool try_push(Task &task) {
    {
      std::unique_lock<std::mutex> lock{mutex_, std::try_to_lock};
      if (!lock)
        return false;
      task.save_arrival_time();
      queue_.emplace_back(task);

      // Is the queue bounded?
      if (queue_size::is_bounded) {
        while (queue_.size() > queue_size::value) {
          // Queue size greater than bound
          if (queue_size::remove_policy == remove_task::newest) {
            queue_.pop_back(); // newest task is in the back of the queue
          } else if (queue_size::remove_policy == remove_task::oldest) {
            queue_.pop_front(); // oldest task is in the front of the queue
          }
        }
      }
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

  template <class A> bool try_pop_if_starved(Task &task) {
    std::unique_lock<std::mutex> lock{mutex_, std::try_to_lock};
    if (!lock || queue_.empty())
      return false;
    task = queue_.front();
    const auto now = std::chrono::steady_clock::now();
    const auto diff = std::chrono::duration_cast<typename A::type>(now - task.stats_.arrival_time);
    if (diff > A::value) {
      // pop the task so it can be enqueued at a higher priority
      task = std::move(queue_.front());
      queue_.pop_front();
      return true;
    }
    return false;
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

template <size_t P> struct increment_priority { constexpr static size_t value = P; };

template <typename T> struct is_chrono_duration { static constexpr bool value = false; };

template <typename Rep, typename Period>
struct is_chrono_duration<std::chrono::duration<Rep, Period>> {
  static constexpr bool value = true;
};

template <class D, size_t P> struct task_starvation_after {
  static_assert(is_chrono_duration<D>::value, "Duration must be a std::chrono::duration");
  typedef D type;
  constexpr static D value = D(P);
};

template <class threads, class priority_levels, class task_starvation_after,
          class maintain_queue_size = maintain_queue_size<>>
class PriorityScheduler {
  std::vector<std::thread> threads_; // Scheduler thread pool
  std::array<TaskQueue<maintain_queue_size>, priority_levels::value>
      priority_queues_;              // Array of task queues
  std::atomic_bool running_{false};  // Is the scheduler running?
  std::mutex mutex_;                 // Mutex to protect `enqueued_`
  std::condition_variable ready_;    // Signal to notify task enqueued
  std::atomic_bool enqueued_{false}; // Set to true when a task is scheduled

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

      // Handle task starvation at lower priorities
      // Modulate priorities based on age
      // Start from the lowest priority till (highest_priority - 1)
      for (size_t i = 0; i < priority_levels::value - 1; i++) {
        // Check if the front of the queue has a starving task
        if (priority_queues_[i].template try_pop_if_starved<task_starvation_after>(task)) {
          // task has been starved, reschedule at a higher priority
          while (running_) {
            if (priority_queues_[i + 1].try_push(task)) {
              break;
            }
          }
        }
      }

      // Run the highest priority ready task
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

  template <class priority> void schedule(Task &task) {
    static_assert(priority::value <= priority_levels::value, "priority out of range");

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
