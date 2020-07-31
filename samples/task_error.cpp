#include <iostream>
#include <psched/priority_scheduler.h>
using namespace psched;

int main() {

  PriorityScheduler<threads<1>, priority_levels<1>> scheduler;

  Task fail(
      // Task action
      [] { throw std::runtime_error("Task Error: Uh oh, something bad happened"); },

      // Task post-completion callback
      {},

      // Task error callback
      [](const char *error_message) { std::cout << error_message << "\n"; });

  scheduler.schedule<priority<0>>(fail);
}