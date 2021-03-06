
#pragma once
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <psched/queue_size.h>
#include <psched/task.h>

namespace psched {

template <class queue_policy> class TaskQueue {
  std::deque<Task> queue_;        // Internal queue data structure
  bool done_{false};              // Set to true when no more tasks are expected
  std::mutex mutex_;              // Mutex for the internal queue
  std::condition_variable ready_; // Signal for when a task is enqueued

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

      // Is the queue bounded?
      if (queue_policy::bounded_or_not) {
        while (queue_.size() > queue_policy::maintain_size::bounded_queue_size) {
          // Queue size greater than bound
          if (queue_policy::maintain_size::discard_policy == discard::newest_task) {
            queue_.pop_back(); // newest task is in the back of the queue
          } else if (queue_policy::maintain_size::discard_policy == discard::oldest_task) {
            queue_.pop_front(); // oldest task is in the front of the queue
          }
        }
      }
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