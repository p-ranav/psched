
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
#include <stddef.h>

namespace psched {

enum class discard { oldest_task, newest_task };

template <size_t queue_size, discard policy> struct maintain_size {
  constexpr static size_t bounded_queue_size = queue_size;
  constexpr static discard discard_policy = policy;
};

template <size_t count, class M = maintain_size<0, discard::oldest_task>> struct queues {
  constexpr static bool bounded_or_not = (M::bounded_queue_size > 0);
  constexpr static size_t number_of_queues = count;
  typedef M maintain_size;
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
// #include <psched/queue_size.h>
// #include <psched/task.h>

namespace psched {

template <class queue_policy> class TaskQueue {
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
      if (queue_policy::bounded_or_not) {
        while (queue_.size() > queue_policy::maintain_size::bounded_queue_size) {
          // Queue size greater than bound
          if (queue_policy::maintain_size::discard_policy == discard::newest_task) {
            queue_.pop_back(); // newest task is in the back of the queue
          } else if (queue_policy::maintain_size::discard_policy == discard::oldest_task) {
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
#include <chrono>

namespace psched {

template <typename T> struct is_chrono_duration { static constexpr bool value = false; };

template <typename Rep, typename Period>
struct is_chrono_duration<std::chrono::duration<Rep, Period>> {
  static constexpr bool value = true;
};

template <class D = std::chrono::milliseconds, size_t P = 0> struct task_starvation_after {
  static_assert(is_chrono_duration<D>::value, "Duration must be a std::chrono::duration");
  typedef D type;
  constexpr static D value = D(P);
};

template <size_t P> struct increment_priority_by { constexpr static size_t value = P; };

template <class T = task_starvation_after<>, class I = increment_priority_by<1>>
struct aging_policy {
  typedef T task_starvation_after;
  typedef I increment_priority_by;
};

} // namespace psched
#pragma once
#include <array>
#include <atomic>
#include <condition_variable>
#include <mutex>
// #include <psched/aging_policy.h>
// #include <psched/task.h>
// #include <psched/task_queue.h>
#include <thread>
#include <vector>

namespace psched {

template <size_t T> struct threads { constexpr static size_t value = T; };

template <size_t P> struct priority { constexpr static size_t value = P; };

template <class threads, class queues, class aging_policy> class PriorityScheduler {
  constexpr static size_t priority_levels = queues::number_of_queues;

  std::vector<std::thread> threads_{};                               // Scheduler thread pool
  std::array<TaskQueue<queues>, priority_levels> priority_queues_{}; // Array of task queues
  std::atomic_bool running_{false};                                  // Is the scheduler running?
  std::mutex mutex_{};                                               // Mutex to protect `enqueued_`
  std::condition_variable ready_{}; // Signal to notify task enqueued
  std::atomic_size_t enqueued_{0};  // Incremented when a task is scheduled

  void run() {
    while (running_ || enqueued_ > 0) {
      // Wait for the `enqueued` signal
      {
        std::unique_lock<std::mutex> lock{mutex_};
        ready_.wait(lock, [this] { return enqueued_ > 0 || !running_; });
      }

      Task t;

      // // Handle task starvation at lower priorities
      // // Modulate priorities based on age
      // // Start from the lowest priority till (highest_priority - 1)
      // for (size_t i = 0; i < priority_levels - 1; i++) {
      //   // Check if the front of the queue has a starving task
      //   if (priority_queues_[i]
      //           .template try_pop_if_starved<typename aging_policy::task_starvation_after>(t)) {
      //     // task has been starved, reschedule at a higher priority
      //     while (running_) {
      //       const auto new_priority =
      //           std::min(i + aging_policy::increment_priority_by::value, priority_levels - 1);
      //       if (priority_queues_[new_priority].try_push(t)) {
      //         break;
      //       }
      //     }
      //   }
      // }

      // Run the highest priority ready task
      bool dequeued = false;

      while (!dequeued) {
        for (size_t i = priority_levels; i > 0; --i) {
          // Try to pop an item
          if (priority_queues_[i - 1].try_pop(t)) {
            dequeued = true;
            if (enqueued_ > 0) {
              enqueued_ -= 1;
            }
            // execute task
            t();
            break;
          }
        }
        if (!(running_ || enqueued_ > 0))
          break;
      }
    }
  }

public:
  PriorityScheduler() {
    running_ = true;
    for (unsigned n = 0; n != threads::value; ++n) {
      threads_.emplace_back([&, n] { run(); });
    }
  }

  ~PriorityScheduler() {
    running_ = false;
    ready_.notify_all();
    for (auto &q : priority_queues_)
      q.done();
    for (auto &t : threads_)
      if (t.joinable())
        t.join();
  }

  template <class priority> void schedule(Task &task) {
    static_assert(priority::value <= priority_levels, "priority out of range");

    // Enqueue task
    while (running_) {
      if (priority_queues_[priority::value].try_push(task)) {
        break;
      }
    }

    // Send `enqueued` signal to worker threads
    {
      std::unique_lock<std::mutex> lock{mutex_};
      enqueued_ += 1;
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
