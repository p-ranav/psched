#include <iostream>
#include <psched/priority_scheduler.h>
using namespace psched;

int main() {
  // Initialize the scheduler
  PriorityScheduler<threads<4>, priority_levels<1>> scheduler;

  // Configure a task
  Task t([] { std::cout << "Hello World\n"; });

  // Schedule the task
  scheduler.schedule<priority<0>>(t);
}