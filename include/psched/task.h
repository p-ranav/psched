
#pragma once
#include <atomic>
#include <functional>
#include <psched/task_functions.h>

namespace psched {

class Task {
  TaskFunctions functions_;
  TaskStats stats_;

  template <class threads, class priority_levels> friend class PriorityScheduler;

protected:
  void save_arrival_time() { stats_.arrival_time = std::chrono::steady_clock::now(); }

public:
  Task(const std::function<void()> &task_on_execute = {},
       const std::function<void(const TaskStats &)> &task_on_complete = {},
       const std::function<void(const TaskStats &, const char *)> &task_on_error = {})
      : functions_(task_on_execute, task_on_complete, task_on_error) {}

  Task(const Task &other) {
    functions_ = other.functions_;
    stats_ = other.stats_;
  }

  Task &operator=(Task other) {
    std::swap(functions_, other.functions_);
    std::swap(stats_, other.stats_);
    return *this;
  }

  void on_execute(const std::function<void()> &fn) { functions_.task_main = fn; }

  void on_complete(const std::function<void(const TaskStats &)> &fn) { functions_.task_end = fn; }

  void on_error(const std::function<void(const TaskStats &, const char *)> &fn) {
    functions_.task_error = fn;
  }

  void operator()() {
    stats_.start_time = std::chrono::steady_clock::now();
    try {
      if (functions_.task_main) {
        functions_.task_main();
      }
      stats_.end_time = std::chrono::steady_clock::now();
    } catch (std::exception &e) {
      stats_.end_time = std::chrono::steady_clock::now();
      if (functions_.task_error) {
        functions_.task_error(stats_, e.what());
      }
    } catch (...) {
      stats_.end_time = std::chrono::steady_clock::now();
      if (functions_.task_error) {
        functions_.task_error(stats_, "Unknown exception");
      }
    }
    if (functions_.task_end) {
      functions_.task_end(stats_);
    }
  }
};

} // namespace psched