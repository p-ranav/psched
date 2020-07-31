
#pragma once
#include <array>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <psched/task.h>
#include <psched/task_queue.h>
#include <thread>
#include <vector>

namespace psched {

template <size_t T> struct threads { constexpr static size_t value = T; };

template <size_t P> struct priority_levels { constexpr static size_t value = P; };

template <size_t P> struct priority { constexpr static size_t value = P; };

template <class D, size_t P> struct period { constexpr static D value = D(P); };

template <class threads, class priority_levels> class PriorityScheduler {
  std::vector<std::thread> threads_;
  std::array<TaskQueue, priority_levels::value> priority_queues_;
  std::atomic_bool running_{false};
  std::mutex mutex_;
  std::condition_variable ready_;
  std::atomic_bool enqueued_{false};

  void run() {
    while (running_) {
      // Wait for the `enqueued` signal
      {
        std::unique_lock<std::mutex> lock{mutex_};
        ready_.wait(lock, [this] { return enqueued_.load() || !running_; });
        enqueued_ = false;
      }

      if (!running_) {
        break;
      }

      Task task;
      bool dequeued = false;

      // Start from highest priority queue
      do {
        for (size_t i = priority_levels::value - 1; i >= 0; i--) {
          // Try to pop an item
          if (priority_queues_[i].try_pop(task)) {
            dequeued = true;
            break;
          }
        }
      } while (!dequeued && running_);

      // execute task
      task();
    }
  }

public:
  PriorityScheduler() {
    running_ = true;
    for (unsigned n = 0; n != threads::value; ++n) {
      threads_.emplace_back([this] { run(); });
    }
  }

  ~PriorityScheduler() {
    for (auto &q : priority_queues_)
      q.done();
    for (auto &t : threads_)
      if (t.joinable())
        t.join();
  }

  template <class priority> void schedule(Task &task) {
    static_assert(priority::value <= priority_levels::value, "priority out of range");

    // Save task arrival time
    task.save_arrival_time();

    // Enqueue task
    while (running_) {
      if (priority_queues_[priority::value].try_push(task)) {
        break;
      }
    }

    // Send `enqueued` signal to worker threads
    {
      std::unique_lock<std::mutex> lock{mutex_};
      enqueued_ = true;
      ready_.notify_one();
    }
  }

  template <class priority, class period> void schedule(Task& task) {
    std::thread([this, &task]() {
      do {
        // schedule task at priority level 2
        schedule<priority>(task);

        // sleep for 100ms
        std::this_thread::sleep_for(period::value);
      } while (true);
    }).detach();
  }

  void stop() {
    running_ = false;
    ready_.notify_all();
    for (auto &q : priority_queues_)
      q.done();
    for (auto &t : threads_)
      if (t.joinable())
        t.join();
  }
};

} // namespace psched
