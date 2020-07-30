#include <iostream>
#include <psched/priority_scheduler.h>
using namespace psched;

/*
| Task | Period (ms) | Burst Time (ms) | Priority    |
|------|-------------|-----------------|-------------|
| a    |  250        | 130             | 0 (Lowest)  |
| b    |  500        | 390             | 1           |
| c    | 1000        | 560             | 2 (Highest) |
*/

int main() {
  PriorityScheduler<threads<2>, priority_levels<3>> scheduler;

  Task a(
    // Task action
    [] {
      std::this_thread::sleep_for(std::chrono::milliseconds(130));
    },
    // Task post-completion callback
    [](const TaskStats& stats) {
      std::cout << "[Task a] ";
      std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
      std::cout << "Burst time = " << stats.burst_time() << "ms; ";
      std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
    }
  );

  Task b(
    // Task action
    [] {
      std::this_thread::sleep_for(std::chrono::milliseconds(390));
    },
    // Task post-completion callback
    [](const TaskStats& stats) {
      std::cout << "[Task b] ";
      std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
      std::cout << "Burst time = " << stats.burst_time() << "ms; ";
      std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
    }
  );

  Task c(
    // Task action
    [] {
      std::this_thread::sleep_for(std::chrono::milliseconds(560));
    },
    // Task post-completion callback
    [](const TaskStats& stats) {
      std::cout << "[Task c] ";
      std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
      std::cout << "Burst time = " << stats.burst_time() << "ms; ";
      std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
    }
  );

  // Schedule the tasks
  scheduler.schedule<priority<0>, period<std::chrono::milliseconds, 250>>(a);
  scheduler.schedule<priority<1>, period<std::chrono::milliseconds, 500>>(b);
  scheduler.schedule<priority<2>, period<std::chrono::milliseconds, 1000>>(c);
}