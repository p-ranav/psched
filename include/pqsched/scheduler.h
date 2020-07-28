#pragma once
#include <pqsched/priority_queue.h>
#include <array>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>

namespace pqsched {

template <typename Task, size_t PriorityLevels>
class scheduler {
  const unsigned count_{std::thread::hardware_concurrency()};
  std::vector<std::thread> threads_;
  std::array<queue<Task>, PriorityLevels> priority_queues_;
  std::array<std::mutex, PriorityLevels> mutex_;

  void run() {
    while (true) {
      Task task;
      bool dequeued = false;

      // Start from highest priority queue
      for (size_t i = 0; i < PriorityLevels; i++) {
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
      std::cout << "Dequeued task: " << task << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
  }

public:

  scheduler() {
    for (unsigned n = 0; n != count_; ++n) {
      threads_.emplace_back([this] { run(); });
    }
  }

  ~scheduler() {
    for (auto& q: priority_queues_)
      q.done();
    for (auto& t: threads_)
      t.join();
  }
  
  void schedule(Task&& task, size_t priority) {
    lock_t lock{mutex_[priority]};
    while(true) {
      if (priority_queues_[priority].try_push(std::forward<Task>(task)))
	break;
    }
    print_queues();
  }

  void print_queues() {
    size_t priority_level = 0;
    std::cout << "\n";
    for (auto& q : priority_queues_) {
      std::cout << priority_level << " ";
      q.print_queue();
      priority_level += 1;
    }
    std::cout << "\n";
  }
};
  
}
