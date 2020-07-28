#pragma once
#include <atomic>
#include <functional>
#include <iostream>

namespace pqsched {

class Task {
  size_t id_;
  size_t priority_;
  std::function<void()> fn_;
  std::atomic_bool done_{false};
public:

  Task() : id_(0), priority_(0) {}

  Task(const Task & other) {
    id_ = other.id_;
    priority_ = other.priority_;
    fn_ = other.fn_;
  }

  Task& operator=(Task other) {
    std::swap(id_, other.id_);
    std::swap(priority_, other.priority_);
    std::swap(fn_, other.fn_);
    return *this;
  }

  void set_id(size_t id) {
    id_ = id;
  }

  void set_priority(size_t priority) {
    priority_ = priority;
  }

  template <typename Function>
  void set_function(Function&& fn) {
    fn_ = std::forward<Function>(fn);
  }

  void operator()() {
    try {
      fn_();
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