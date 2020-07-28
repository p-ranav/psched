#pragma once
#include <array>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <pqsched/queue.h>
#include <thread>

namespace pqsched {

template <size_t priority> struct priority_levels {
  constexpr static char value = priority;
};

template <typename Task, class priority_levels> class scheduler {
  const unsigned count_{std::thread::hardware_concurrency()};
  std::vector<std::thread> threads_;
  std::array<queue<Task>, priority_levels::value> priority_queues_;
  std::array<std::mutex, priority_levels::value> mutex_;

  void run() {
    while (true) {

      // Remove sleep when you actually do something with task
      std::this_thread::sleep_for(std::chrono::seconds(1));


      Task task;
      bool dequeued = false;

      // Start from highest priority queue
      for (size_t i = 0; i < priority_levels::value; i++) {
        // Try to pop an item
        lock_t lock{mutex_[i]};
        if (priority_queues_[i].try_pop(task)) {
          dequeued = true;
          break;
        }
      }

      if (!dequeued)
        continue;

      // do something with task
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

  void schedule(Task &&task, size_t priority) {
    lock_t lock{mutex_[priority]};
    while (true) {
      if (priority_queues_[priority].try_push(std::forward<Task>(task)))
        break;
    }
    print_queues();
  }

  void print_queues() {
    size_t priority_level = 0;
    std::cout << "\n";
    for (auto &q : priority_queues_) {
      std::cout << "P" << priority_level << " ";
      q.print_queue();
      priority_level += 1;
    }
    std::cout << "\n";
  }
};

} // namespace pqsched
