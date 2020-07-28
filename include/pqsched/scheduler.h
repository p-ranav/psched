#pragma once
#include <array>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <pqsched/queue.h>
#include <thread>

namespace pqsched {

using Task = std::function<void()>;

template <size_t priority_levels> class scheduler {
  const unsigned count_{std::thread::hardware_concurrency()};
  std::vector<std::thread> threads_;
  std::array<queue<Task>, priority_levels> priority_queues_;
  std::array<std::mutex, priority_levels> mutex_;

  void run() {
    while (true) {
      Task task;
      bool dequeued = false;

      // Start from highest priority queue
      for (size_t i = 0; i < priority_levels; i++) {
        // Try to pop an item
        lock_t lock{mutex_[i]};
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
  scheduler() {
    for (unsigned n = 0; n != count_; ++n) {
      threads_.emplace_back([this] { run(); });
    }
  }

  ~scheduler() {
    for (auto &q : priority_queues_)
      q.done();
    for (auto &t : threads_)
      t.join();
  }

  void schedule(Task &&fn, size_t priority) {
    lock_t lock{mutex_[priority]};
    while (true) {
      if (priority_queues_[priority].try_push(std::forward<Task>(fn)))
        break;
    }
  }

};

} // namespace pqsched
