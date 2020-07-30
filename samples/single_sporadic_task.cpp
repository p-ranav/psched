#include <iostream>
#include <psched/priority_scheduler.h>
using namespace psched;

int main() {
  // Initialize the scheduler
  PriorityScheduler<threads<1>, priority_levels<1>> scheduler;
  scheduler.start();

  // Configure a task - This one's sporadic
  Task t;
  t.on_execute([] { std::cout << "Hello World" << std::endl; });

  // Schedule the task at some priority level, e.g., 3
  scheduler.schedule(t, 0);
}