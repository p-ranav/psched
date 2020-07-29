#pragma once
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <pqsched/concurrent_queue.h>
#include <pqsched/task.h>

namespace pqsched {

class TaskQueue {
  moodycamel::ConcurrentQueue<Task> queue_;
  // std::mutex mutex_;
  // bool done_{false};
  // std::condition_variable ready_;

public:
  bool try_pop(Task &task) {
    // std::unique_lock<std::mutex> lock{mutex_, std::try_to_lock};
    // if (!lock)
    //   return false;
    return queue_.try_dequeue(task);
    // task = std::move(queue_.front());
    // queue_.pop_front();
    // return true;
  }

  bool try_push(Task &task) {
    task.save_arrival_time();
    return queue_.enqueue(task);
    // {
    //   std::unique_lock<std::mutex> lock{mutex_, std::try_to_lock};
    //   if (!lock)
    //     return false;
    //   task.save_arrival_time(); // task about to be enqueued. Mark as ready here
    //   queue_.emplace_back(task);
    // }
    // ready_.notify_one();
    // return true;
  }

  // void done() {
  //   {
  //     std::unique_lock<std::mutex> lock{mutex_};
  //     done_ = true;
  //   }
  //   ready_.notify_all();
  // }

};

} // namespace pqsched
