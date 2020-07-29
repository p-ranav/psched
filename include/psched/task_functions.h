#pragma once
#include <functional>
#include <pqsched/task_stats.h>

namespace psched {

struct TaskFunctions {

  // Called when the task is (finally) executed by an executor thread
  std::function<void()> task_main;

  // Called after the task has completed executing.
  // In case of exception, `task_error` is called first
  //
  // TaskStats argument can be used to get task computation_time
  // and task response_time.
  std::function<void(TaskStats)> task_end;

  // Called if `task_main()` throws an exception
  std::function<void(TaskStats, const char *)> task_error;
};

}