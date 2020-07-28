#pragma once
#include <mutex>
#include <queue>
#include <condition_variable>
#include <iostream>

using lock_t = std::unique_lock<std::mutex>;

namespace pqsched {

template <typename Task>
class queue {
  std::deque<Task> queue_;
  std::mutex mutex_;
  bool done_{false};
  std::condition_variable ready_;
  
public:
  bool try_pop(Task &task) {
    lock_t lock{mutex_, std::try_to_lock};
    if (!lock || queue_.empty())
      return false;
    task = std::move(queue_.front());
    queue_.pop_front();
    return true;
  }

  bool try_push(Task &&task) {
    {
      lock_t lock{mutex_, std::try_to_lock};
      if (!lock)
	return false;
      queue_.emplace_back(std::forward<Task>(task));
    }
    ready_.notify_one();
    return true;
  }

  void done() {
    {
      lock_t lock{mutex_};
      done_ = true;
    }
    ready_.notify_all();
  }

  bool pop(Task &task) {
    lock_t lock{mutex_};
    while (queue_.empty())
      ready_.wait(lock);
    if (queue_.empty())
      return false;
    task = std::move(queue_.front());
    queue_.pop_front();
    return true;
  }

  void push(Task &&task) {
    {
      lock_t lock{mutex_};
      queue_.emplace_back(std::forward<Task>(task));
    }
    ready_.notify_one();
  }

  void print_queue() {
    std::cout << "| ";
    for (auto& q: queue_) {
      std::cout << q << " | ";
    }
    std::cout << "\n";
  }
  
};

}
