#pragma once
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <psched/concurrent_queue.h>
#include <psched/task.h>

namespace psched {

class TaskQueue {
  moodycamel::ConcurrentQueue<Task> queue_;

public:
  bool try_pop(Task &task) {
    return queue_.try_dequeue(task);
  }

  bool try_push(Task &task) {
    task.save_arrival_time();
    return queue_.enqueue(task);
  }
};

} // namespace psched
