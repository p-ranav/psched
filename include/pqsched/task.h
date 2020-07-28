#pragma once
#include <atomic>
#include <functional>
#include <pqsched/task_functions.h>
#include <pqsched/task_stats.h>

namespace pqsched {

class Task {
  size_t id_;
  size_t priority_;
  TaskFunctions functions_;
  TaskStats stats_;
  std::atomic_bool done_{false};

  friend class TaskQueue;

protected:
  void save_arrival_time() {
    stats_.arrival_time = std::chrono::steady_clock::now();
  }

public:

  Task() : id_(0), priority_(0) {}

  Task(const Task & other) {
    id_ = other.id_;
    priority_ = other.priority_;
    functions_ = other.functions_;
    stats_ = other.stats_;
  }

  Task& operator=(Task other) {
    std::swap(id_, other.id_);
    std::swap(priority_, other.priority_);
    std::swap(functions_, other.functions_);
    std::swap(stats_, other.stats_);
    return *this;
  }

  void set_id(size_t id) {
    id_ = id;
  }

  void set_priority(size_t priority) {
    priority_ = priority;
  }

  template <typename Function>
  void on_execute(Function&& fn) {
    functions_.task_main = std::forward<Function>(fn);
  }

  template <typename Function>
  void on_complete(Function&& fn) {
    functions_.task_end = std::forward<Function>(fn);
  }

  template <typename Function>
  void on_error(Function&& fn) {
    functions_.task_error = std::forward<Function>(fn);
  }

  void operator()() {
    try {
      stats_.start_time = std::chrono::steady_clock::now();
      if (functions_.task_main) {
        functions_.task_main();
      }
      stats_.end_time = std::chrono::steady_clock::now();
      done_ = true;
    } 
    catch (std::exception & e) {
      stats_.end_time = std::chrono::steady_clock::now();
      if (functions_.task_error) {
        functions_.task_error(stats_, e.what());
      }
    }
    catch (...) {
      stats_.end_time = std::chrono::steady_clock::now();
      if (functions_.task_error) {
        functions_.task_error(stats_, "Unknown exception");
      }
    }
    if (functions_.task_end) {
      functions_.task_end(stats_);
    }
  }

  bool is_done() const {
    return done_;
  }

  size_t get_priority() const {
    return priority_;
  }

};

}