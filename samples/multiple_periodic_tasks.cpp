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
  PriorityScheduler<
      threads<3>, priority_levels<3>,
      aging_policy<task_starvation_after<std::chrono::milliseconds, 250>, increment_priority_by<1>>,
      maintain_queue_size<100, remove_task::oldest>>
      scheduler;

  Task a(
      // Task action
      [] { std::this_thread::sleep_for(std::chrono::milliseconds(130)); },
      // Task post-completion callback
      [](const TaskStats &stats) {
        std::cout << "[Task a] ";
        std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
        std::cout << "Burst time = " << stats.burst_time() << "ms; ";
        std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
      });

  auto timer_a = std::thread([&scheduler, &a]() {
    while (true) {
      scheduler.schedule<priority<0>>(a);
      std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
  });

  Task b(
      // Task action
      [] { std::this_thread::sleep_for(std::chrono::milliseconds(390)); },
      // Task post-completion callback
      [](const TaskStats &stats) {
        std::cout << "[Task b] ";
        std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
        std::cout << "Burst time = " << stats.burst_time() << "ms; ";
        std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
      });

  auto timer_b = std::thread([&scheduler, &b]() {
    while (true) {
      scheduler.schedule<priority<1>>(b);
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  });

  Task c(
      // Task action
      [] { std::this_thread::sleep_for(std::chrono::milliseconds(560)); },
      // Task post-completion callback
      [](const TaskStats &stats) {
        std::cout << "[Task c] ";
        std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
        std::cout << "Burst time = " << stats.burst_time() << "ms; ";
        std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
      });

  auto timer_c = std::thread([&scheduler, &c]() {
    while (true) {
      scheduler.schedule<priority<2>>(c);
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
  });

  timer_a.join();
  timer_b.join();
  timer_c.join();
}