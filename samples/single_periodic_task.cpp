#include <iostream>
#include <psched/priority_scheduler.h>
using namespace psched;

int main() {

  // Initialize scheduler
  PriorityScheduler<threads<2>, priority_levels<3>> scheduler;

  // Configure task
  Task t;
  t.on_execute([] {
    // execution time (burst time) of task = 40ms
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
  });

  t.on_complete([](const TaskStats& stats) {
    std::cout << "Timer 1 fired! ";
    std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
    std::cout << "Burst time = " << stats.burst_time() << "ms; ";
    std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
  });

  // Schedule task periodically
  auto timer1 = std::thread([&scheduler, &t]() {
    do {
      // schedule task at priority level 2
      scheduler.schedule<priority<2>>(t);

      // sleep for 100ms
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (true);
  });

  timer1.join();
}