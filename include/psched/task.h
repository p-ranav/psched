
#pragma once
#include <atomic>
#include <functional>
#include <psched/task_stats.h>

namespace psched {

class Task {
  // Called when the task is (finally) executed by an executor thread
  std::function<void()> task_main_;

  // Called after the task has completed executing.
  // In case of exception, `task_error` is called first
  //
  // TaskStats argument can be used to get task computation_time
  // and task response_time.
  std::function<void(const TaskStats &)> task_end_;

  // Called if `task_main()` throws an exception
  std::function<void(const char *)> task_error_;

  TaskStats stats_;

  template <class threads, class priority_levels> friend class PriorityScheduler;

protected:
  void save_arrival_time() { stats_.arrival_time = std::chrono::steady_clock::now(); }

public:
  Task(const std::function<void()> &task_main = {},
       const std::function<void(const TaskStats &)> &task_end = {},
       const std::function<void(const char *)> &task_error = {})
      : task_main_(task_main), task_end_(task_end), task_error_(task_error) {}

  Task(const Task &other) {
    task_main_ = other.task_main_;
    task_end_ = other.task_end_;
    task_error_ = other.task_error_;
    stats_ = other.stats_;
  }

  Task &operator=(Task other) {
    std::swap(task_main_, other.task_main_);
    std::swap(task_end_, other.task_end_);
    std::swap(task_error_, other.task_error_);
    std::swap(stats_, other.stats_);
    return *this;
  }

  void on_execute(const std::function<void()> &fn) { task_main_ = fn; }

  void on_complete(const std::function<void(const TaskStats &)> &fn) { task_end_ = fn; }

  void on_error(const std::function<void(const char *)> &fn) {
    task_error_ = fn;
  }

  void operator()() {
    stats_.start_time = std::chrono::steady_clock::now();
    try {
      if (task_main_) {
        task_main_();
      }
      stats_.end_time = std::chrono::steady_clock::now();
    } catch (std::exception &e) {
      stats_.end_time = std::chrono::steady_clock::now();
      if (task_error_) {
        task_error_(e.what());
      }
    } catch (...) {
      stats_.end_time = std::chrono::steady_clock::now();
      if (task_error_) {
        task_error_("Unknown exception");
      }
    }
    if (task_end_) {
      task_end_(stats_);
    }
  }
};

} // namespace psched