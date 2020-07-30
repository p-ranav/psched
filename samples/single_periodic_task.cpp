#include <iostream>
#include <psched/priority_scheduler.h>
using namespace psched;

int main() {

  // Initialize scheduler
  PriorityScheduler<threads<2>, priority_levels<1>> scheduler;

  // Configure task
  Task task
  (
    // Task action
    [] {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
    },
    // Task post-completion callback
    [](const TaskStats& stats) {
      std::cout << "Timer 1 fired! ";
      std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
      std::cout << "Burst time = " << stats.burst_time() << "ms; ";
      std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
    }
  );

  // Schedule periodic task
  scheduler.schedule<priority<0>, 
                     period<std::chrono::milliseconds, 100>>(task);
}