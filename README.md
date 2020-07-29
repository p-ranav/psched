# psched

`psched` is a lightweight library that provides a priority-based task scheduler for modern C++.

## Getting Started

* The priority scheduler manages an array of concurrent queues, each queue assigned a priority-level
* A task, when scheduled, is enqueued onto one of queues based on the task's priority
* A pool of threads executes ready tasks, starting with the highest priority

<p align="center">
  <img height="400" src="img/priority_scheduling.png"/>  
</p>

## Simple Periodic Task

* The following scheduler uses 4 worker threads and 5 queues (for the 5 priority levels). 
  - Priority 0 is the highest priority
  - Priority 4 is the lowest priority

```cpp
#include <psched/priority_scheduler.h>
using namespace psched;

int main() {
  // Initialize scheduler
  PriorityScheduler<threads<4>, priority_levels<5>> scheduler;
  scheduler.start();

  // Configure task
  Task my_task;
  my_task.on_execute([] {
    // execution time of task = 40ms
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
  });

  my_task.on_complete([](TaskStats stats) {
    const auto computation_time = stats.computation_time<std::chrono::milliseconds>();
    const auto response_time = stats.response_time<std::chrono::milliseconds>();
    const auto wait_time = stats.wait_time<std::chrono::milliseconds>();
    std::cout << "Timer 1 fired! ";
    std::cout << "Wait time = " << wait_time << "ms; ";
    std::cout << "Computation time = " << computation_time << "ms; ";
    std::cout << "Response time = " << response_time << "ms\n";
  });

  // Periodic Timer 1 - 100ms period
  auto timer1 = std::thread([&scheduler, &my_task]() {
    while (true) {

      // Schedule task at priority level 3
      scheduler.schedule(my_task, 3);

      // Sleep for 100ms
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  });

  timer1.join();
}
```

## Generating Single Header

```bash
python3 utils/amalgamate/amalgamate.py -c single_include.json -s .
```

## Contributing
Contributions are welcome, have a look at the CONTRIBUTING.md document for more information.

## License
The project is available under the MIT license.
