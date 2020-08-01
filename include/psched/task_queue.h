
#pragma once
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <psched/task.h>

namespace psched {

enum class remove_task { oldest, newest };

template <size_t size = 0, remove_task policy = remove_task::oldest> struct maintain_queue_size {
  constexpr static bool is_bounded = (size > 0);
  constexpr static size_t value = size;
  constexpr static remove_task remove_policy = policy;
};

template <class queue_size = maintain_queue_size<>> class TaskQueue {
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
      if (queue_size::is_bounded) {
        while (queue_.size() > queue_size::value) {
          // Queue size greater than bound
          if (queue_size::remove_policy == remove_task::newest) {
            queue_.pop_back(); // newest task is in the back of the queue
          } else if (queue_size::remove_policy == remove_task::oldest) {
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