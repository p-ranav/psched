
#pragma once
#include <chrono>

namespace psched {

struct TaskStats {
  using TimePoint = std::chrono::steady_clock::time_point;
  TimePoint arrival_time; // time point when the task is marked as 'ready' (queued)
  TimePoint start_time;   // time point when the task is about to execute (dequeued)
  TimePoint end_time;     // time point when the task completes execution

  // Time spent waiting in the queue
  template <typename T> long long wait_time() const {
    return std::chrono::duration_cast<T>(start_time - arrival_time).count();
  }

  // Time taken to execute the task main function (see task_functions.h)
  template <typename T> long long computation_time() const {
    return std::chrono::duration_cast<T>(end_time - start_time).count();
  }

  // wait_time() + computation_time()
  template <typename T> long long response_time() const {
    return std::chrono::duration_cast<T>(end_time - arrival_time).count();
  }
};

} // namespace psched