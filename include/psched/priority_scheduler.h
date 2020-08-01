
#pragma once
#include <array>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <psched/task.h>
#include <psched/task_queue.h>
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

template <class threads, class priority_levels, class task_starvation_after>
class PriorityScheduler {
  std::vector<std::thread> threads_;                              // Scheduler thread pool
  std::array<TaskQueue, priority_levels::value> priority_queues_; // Array of task queues
  std::atomic_bool running_{false};                               // Is the scheduler running?
  std::mutex mutex_;                                              // Mutex to protect `enqueued_`
  std::condition_variable ready_;                                 // Signal to notify task enqueued
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
