#pragma once
#include <array>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <pqsched/task.h>
#include <pqsched/task_queue.h>
#include <thread>

namespace pqsched {

template <size_t priority_levels> class PriorityScheduler {
  const unsigned count_{std::thread::hardware_concurrency()};
  std::vector<std::thread> threads_;
  std::array<TaskQueue, priority_levels> priority_queues_;
  std::array<std::mutex, priority_levels> mutex_;

  void run() {
    while (true) {
      Task task;
      bool dequeued = false;

      // Start from highest priority queue
      for (size_t i = 0; i < priority_levels; i++) {
        // Try to pop an item
        std::unique_lock<std::mutex> lock{mutex_[i]};
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
  PriorityScheduler() {
    for (unsigned n = 0; n != count_; ++n) {
      threads_.emplace_back([this] { run(); });
    }
  }

  ~PriorityScheduler() {
    for (auto &q : priority_queues_)
      q.done();
    for (auto &t : threads_)
      t.join();
  }

  void schedule(Task & task) {
    const size_t priority = task.get_priority();
    std::unique_lock<std::mutex> lock{mutex_[priority]};
    while (true) {
      if (priority_queues_[priority].try_push(task))
        break;
    }
  }

};

} // namespace pqsched
