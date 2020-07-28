#pragma once
#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <tuple>

namespace pqsched {

using TimePoint = std::chrono::steady_clock::time_point;

struct TaskStats {
  TimePoint arrival_time;
  TimePoint start_time;
  TimePoint end_time;

  template <typename T>
  long long response_time() const {
    return std::chrono::duration_cast<T>(end_time - arrival_time).count();
  }

  template <typename T>
  long long computation_time() const {
    return std::chrono::duration_cast<T>(end_time - start_time).count();
  }

};

class Task {
  size_t id_;
  size_t priority_;
  std::function<void()> execute_fn_;
  std::function<void(TaskStats)> completed_fn_;
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
    execute_fn_ = other.execute_fn_;
    completed_fn_ = other.completed_fn_;
    stats_ = other.stats_;
  }

  Task& operator=(Task other) {
    std::swap(id_, other.id_);
    std::swap(priority_, other.priority_);
    std::swap(execute_fn_, other.execute_fn_);
    std::swap(completed_fn_, other.completed_fn_);
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
    execute_fn_ = std::forward<Function>(fn);
  }

  template <typename Function>
  void on_complete(Function&& fn) {
    completed_fn_ = std::forward<Function>(fn);
  }

  void operator()() {
    try {
      stats_.start_time = std::chrono::steady_clock::now();
      if (execute_fn_)
        execute_fn_();
      stats_.end_time = std::chrono::steady_clock::now();
      if (completed_fn_)
        completed_fn_(stats_);
      done_ = true;
    } 
    catch (std::exception & e) {
      std::cout << e.what() << "\n";
    }
    catch (...) {
      std::cout << "Unknown exception while executing task " << id_ << "\n";
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