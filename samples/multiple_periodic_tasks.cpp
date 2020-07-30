#include <iostream>
#include <psched/priority_scheduler.h>
using namespace psched;

int main() {
  PriorityScheduler<threads<5>, priority_levels<5>> scheduler;
  scheduler.start();

  {
    // Periodic Timer 1 - 50ms period
    auto timer1 = std::thread([&scheduler]() {
      while (true) {
        // Sleep for 50ms
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Generate task
        Task t;
        t.on_execute([] {
          // execution time of task = 40ms
          std::this_thread::sleep_for(std::chrono::milliseconds(40));
        });
        t.on_complete([](TaskStats stats) {
          std::cout << "[Lowest ] ";
          std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
          std::cout << "Burst time = " << stats.burst_time() << "ms; ";
          std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
        });
        scheduler.schedule(t, 4); // lowest priority
      }
    });

    // Periodic Timer 2 - 100ms period
    auto timer2 = std::thread([&scheduler]() {
      while (true) {
        // Sleep for 50ms
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Generate task
        Task t;
        t.on_execute([] {
          // execution time of task = 80ms
          std::this_thread::sleep_for(std::chrono::milliseconds(80));
        });
        t.on_complete([](TaskStats stats) {
          std::cout << "[Low    ] ";
          std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
          std::cout << "Burst time = " << stats.burst_time() << "ms; ";
          std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
        });
        scheduler.schedule(t, 3); // low priority
      }
    });

    // Periodic Timer 3 - 250ms period
    auto timer3 = std::thread([&scheduler]() {
      while (true) {
        // Sleep for 50ms
        std::this_thread::sleep_for(std::chrono::milliseconds(250));

        // Generate task
        Task t;
        t.on_execute([] {
          // execution time of task = 130ms
          std::this_thread::sleep_for(std::chrono::milliseconds(130));
        });
        t.on_complete([](TaskStats stats) {
          std::cout << "[Medium ] ";
          std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
          std::cout << "Burst time = " << stats.burst_time() << "ms; ";
          std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
        });
        scheduler.schedule(t, 2); // medium priority
      }
    });

    // Periodic Timer 4 - 500ms period
    auto timer4 = std::thread([&scheduler]() {
      while (true) {
        // Sleep for 50ms
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Generate task
        Task t;
        t.on_execute([] {
          // execution time of task = 390ms
          std::this_thread::sleep_for(std::chrono::milliseconds(390));
        });
        t.on_complete([](TaskStats stats) {
          std::cout << "[High   ] ";
          std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
          std::cout << "Burst time = " << stats.burst_time() << "ms; ";
          std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
        });
        scheduler.schedule(t, 1); // high priority
      }
    });

    // Periodic Timer 5 - 1s period
    auto timer5 = std::thread([&scheduler]() {
      while (true) {
        // Sleep for 50ms
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // Generate task
        Task t;
        t.on_execute([] {
          // execution time of task = 560ms
          std::this_thread::sleep_for(std::chrono::milliseconds(560));
        });
        t.on_complete([](TaskStats stats) {
          std::cout << "[Highest] ";
          std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
          std::cout << "Burst time = " << stats.burst_time() << "ms; ";
          std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
        });
        scheduler.schedule(t, 0); // highest priority
      }
    });

    timer1.join();
    timer2.join();
    timer3.join();
    timer4.join();
    timer5.join();
  }
}