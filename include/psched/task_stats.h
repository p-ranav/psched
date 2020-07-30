
#pragma once
#include <chrono>

namespace psched {

struct TaskStats {
  using TimePoint = std::chrono::steady_clock::time_point;
  TimePoint arrival_time; // time point when the task is marked as 'ready' (queued)
  TimePoint start_time;   // time point when the task is about to execute (dequeued)
  TimePoint end_time;     // time point when the task completes execution

  // Waiting time is the amount of time spent by a task waiting
  // in the ready queue for getting the CPU.
  template <typename T = std::chrono::milliseconds> long long waiting_time() const {
    return std::chrono::duration_cast<T>(start_time - arrival_time).count();
  }

  // Burst time is the amount of time required by a task for executing on CPU.
  // It is also called as execution time or running time.
  template <typename T = std::chrono::milliseconds> long long burst_time() const {
    return std::chrono::duration_cast<T>(end_time - start_time).count();
  }

  // Turnaround time (TAT) is the time interval from the time of submission
  // of a task to the time of the completion of the task. It can also be
  // considered as the sum of the time periods spent waiting to get into memory or
  // ready queue, execution on CPU and executing input/output.
  //
  // wait_time() + computation_time()
  template <typename T = std::chrono::milliseconds> long long turnaround_time() const {
    return std::chrono::duration_cast<T>(end_time - arrival_time).count();
  }
};

} // namespace psched