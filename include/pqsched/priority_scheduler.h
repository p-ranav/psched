#pragma once
#include <array>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <pqsched/task.h>
#include <pqsched/task_queue.h>
#include <thread>

namespace pqsched {

template <size_t T> struct threads {
  constexpr static size_t value = T;
};

template <size_t P> struct priority_levels {
  constexpr static size_t value = P;
};

template <class threads, class priority_levels> class PriorityScheduler {
  std::vector<std::thread> threads_;
  std::array<TaskQueue, priority_levels::value> priority_queues_;
  std::atomic_bool running_{false};

  void run() {
    while (running_) {
      Task task;
      bool dequeued = false;

      // Start from highest priority queue
      for (size_t i = 0; i < priority_levels::value; i++) {
        // Try to pop an item
        // std::unique_lock<std::mutex> lock{mutex_[i]};
        if (priority_queues_[i].try_pop(task)) {
          dequeued = true;
          break;
        }
      }

      if (!dequeued)
        continue;

      // execute task
      task();
    }
  }

public:
  // PriorityScheduler() {
  //   for (unsigned n = 0; n != count_; ++n) {
  //     threads_.emplace_back([this] { run(); });
  //   }
  // }

  ~PriorityScheduler() {
    for (auto &t : threads_)
      t.join();
  }

  void schedule(Task & task) {
    const size_t priority = task.get_priority();
    // std::unique_lock<std::mutex> lock{mutex_[priority]};
    while (true) {
      if (priority_queues_[priority].try_push(task)) {
        break;
      }
    }
  }

  void start() {
    running_ = true;
    for (unsigned n = 0; n != threads::value; ++n) {
      std::cout << "Starting thread " << n << "\n";
      threads_.emplace_back([this] { run(); });
    }
  }

  void stop() {
    running_ = false;
  }

};

} // namespace pqsched
