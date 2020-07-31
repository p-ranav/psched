
#pragma once
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <psched/task.h>

namespace psched {

class TaskQueue {
  std::deque<Task> queue_;
  bool done_{false};
  std::mutex mutex_;
  std::condition_variable ready_;

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