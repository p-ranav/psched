
#pragma once
#include <array>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <psched/task.h>
#include <psched/task_queue.h>
#include <thread>

namespace psched {

template <size_t T> struct threads { constexpr static size_t value = T; };

template <size_t P> struct priority_levels { constexpr static size_t value = P; };

template <class threads, class priority_levels> class PriorityScheduler {
  std::vector<std::thread> threads_;
  std::array<TaskQueue, priority_levels::value> priority_queues_;
  std::atomic_bool running_{false};
  std::mutex mutex_;
  std::condition_variable ready_;

  void run() {
    bool first_task = true;
    while (running_) {
      if (first_task) {
        std::unique_lock<std::mutex> lock{mutex_};
        ready_.wait(lock);
        first_task = false;
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
      } while (!dequeued);

      // execute task
      task();

      // Wait for the `enqueued` signal
      std::unique_lock<std::mutex> lock{mutex_};
      ready_.wait(lock);
    }
  }

public:
  ~PriorityScheduler() {
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
    while (running_) {
      if (priority_queues_[priority].try_push(task)) {
        break;
      }
    }
    ready_.notify_one();
  }

  void start() {
    running_ = true;
    for (unsigned n = 0; n != threads::value; ++n) {
      threads_.emplace_back([this] { run(); });
    }
  }

  void stop() {
    running_ = false;
    for (auto &t : threads_)
      if (t.joinable())
        t.join();
  }
};

} // namespace psched
