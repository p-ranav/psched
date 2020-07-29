# psched

`psched` is a lightweight library that provides a priority-based task scheduler for modern C++.

## Getting Started

* The priority scheduler manages an array of concurrent queues, each queue assigned a priority-level
* A task, when scheduled, is enqueued onto one of queues based on the task's priority
* A pool of threads executes ready tasks, starting with the highest priority

<p align="center">
  <img height="400" src="img/priority_scheduling.png"/>  
</p>

## Single Periodic Task

* The following scheduler uses 4 worker threads and 5 queues (for the 5 priority levels). 
  - 0 = Highest Priority, 4 = Lowest Priority

```cpp
#include <psched/priority_scheduler.h>
using namespace psched;

int main() {
  PriorityScheduler<threads<8>, priority_levels<5>> scheduler;
  scheduler.start();

  Task t;
  t.on_execute([] {
    // do work
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
  });

  t.on_complete([](TaskStats stats) {
    std::cout << "Timer 1 fired! ";
    std::cout << "Wait time = " << stats.wait_time() << "ms; ";
    std::cout << "Computation time = " << stats.computation_time() << "ms; ";
    std::cout << "Response time = " << stats.response_time() << "ms\n";
  });

  // Periodic Timer 1 - 100ms period
  auto timer1 = std::thread([&scheduler, &t]() {
    while (true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      scheduler.schedule(t, 3); // schedule at priority level 3
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
