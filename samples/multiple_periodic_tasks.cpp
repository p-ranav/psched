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

  {
    // Periodic Timer 1 - 250ms period
    auto timer1 = std::thread([&scheduler]() {
      while (true) {
        // Sleep for 50ms
        std::this_thread::sleep_for(std::chrono::milliseconds(250));

        // Generate task
        Task a;
        a.on_execute([] {
          // execution time of task = 130ms
          std::this_thread::sleep_for(std::chrono::milliseconds(130));
        });
        a.on_complete([](const TaskStats& stats) {
          std::cout << "[Task a] ";
          std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
          std::cout << "Burst time = " << stats.burst_time() << "ms; ";
          std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
        });
        scheduler.schedule(a, 0); // lowest priority
      }
    });

    // Periodic Timer 2 - 500ms period
    auto timer2 = std::thread([&scheduler]() {
      while (true) {
        // Sleep for 50ms
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Generate task
        Task b;
        b.on_execute([] {
          // execution time of task = 390ms
          std::this_thread::sleep_for(std::chrono::milliseconds(390));
        });
        b.on_complete([](const TaskStats& stats) {
          std::cout << "[Task b] ";
          std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
          std::cout << "Burst time = " << stats.burst_time() << "ms; ";
          std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
        });
        scheduler.schedule(b, 1); // medium priority
      }
    });

    // Periodic Timer 3 - 1s period
    auto timer3 = std::thread([&scheduler]() {
      while (true) {
        // Sleep for 50ms
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // Generate task
        Task c;
        c.on_execute([] {
          // execution time of task = 560ms
          std::this_thread::sleep_for(std::chrono::milliseconds(560));
        });
        c.on_complete([](const TaskStats& stats) {
          std::cout << "[Task c] ";
          std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
          std::cout << "Burst time = " << stats.burst_time() << "ms; ";
          std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
        });
        scheduler.schedule(c, 2); // highest priority
      }
    });

    timer1.join();
    timer2.join();
    timer3.join();
  }
}