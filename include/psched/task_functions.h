
#pragma once
#include <functional>
#include <psched/task_stats.h>

namespace psched {

struct TaskFunctions {
  // Called when the task is (finally) executed by an executor thread
  std::function<void()> task_main;

  // Called after the task has completed executing.
  // In case of exception, `task_error` is called first
  //
  // TaskStats argument can be used to get task computation_time
  // and task response_time.
  std::function<void(const TaskStats &)> task_end;

  // Called if `task_main()` throws an exception
  std::function<void(const TaskStats &, const char *)> task_error;

  TaskFunctions(const std::function<void()> &task_on_execute = {}, 
                const std::function<void(const TaskStats &)> &task_on_complete = {},
                const std::function<void(const TaskStats &, const char *)> &task_on_error = {})
    : task_main(task_on_execute),
      task_end(task_on_complete),
      task_error(task_on_error) {}
};

} // namespace psched