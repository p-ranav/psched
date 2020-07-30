
#pragma once
#include <atomic>
#include <functional>
#include <psched/task_functions.h>

namespace psched {

class Task {
  TaskFunctions functions_;
  TaskStats stats_;
  std::atomic_bool done_{false};

  template <class threads, class priority_levels> friend class PriorityScheduler;

protected:
  void save_arrival_time() { stats_.arrival_time = std::chrono::steady_clock::now(); }

public:
  Task() {}

  Task(const Task &other) {
    functions_ = other.functions_;
    stats_ = other.stats_;
  }

  Task &operator=(Task other) {
    std::swap(functions_, other.functions_);
    std::swap(stats_, other.stats_);
    return *this;
  }

  template <typename Function> void on_execute(Function &&fn) {
    functions_.task_main = std::forward<Function>(fn);
  }

  template <typename Function> void on_complete(Function &&fn) {
    functions_.task_end = std::forward<Function>(fn);
  }

  template <typename Function> void on_error(Function &&fn) {
    functions_.task_error = std::forward<Function>(fn);
  }

  void operator()() {
    stats_.start_time = std::chrono::steady_clock::now();
    try {
      if (functions_.task_main) {
        functions_.task_main();
      }
      stats_.end_time = std::chrono::steady_clock::now();
      done_ = true;
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

  bool is_done() const { return done_; }
};

} // namespace psched