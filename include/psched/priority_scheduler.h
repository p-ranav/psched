
#pragma once
#include <array>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <psched/aging_policy.h>
#include <psched/task.h>
#include <psched/task_queue.h>
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

      // Handle task starvation at lower priorities
      // Modulate priorities based on age
      // Start from the lowest priority till (highest_priority - 1)
      for (size_t i = 0; i < priority_levels - 1; i++) {
        // Check if the front of the queue has a starving task
        if (priority_queues_[i]
                .template try_pop_if_starved<typename aging_policy::task_starvation_after>(t)) {
          // task has been starved, reschedule at a higher priority
          while (running_) {
            const auto new_priority =
                std::min(i + aging_policy::increment_priority_by::value, priority_levels - 1);
            if (priority_queues_[new_priority].try_push(t)) {
              break;
            }
          }
        }
      }

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
      threads_.emplace_back([this] { run(); });
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
