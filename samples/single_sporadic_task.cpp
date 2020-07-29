#include <psched/priority_scheduler.h>
using namespace psched;

int main() {
  // Initialize the scheduler
  PriorityScheduler<threads<4>, priority_levels<10>> scheduler;
  scheduler.start();

  // Configure a task - This one's sporadic
  Task t;
  t.on_execute([] {
    std::cout << "Hello World" << std::endl;
  });

  // Schedule the task at some priority level, e.g., 3
  scheduler.schedule(t, 3);
}