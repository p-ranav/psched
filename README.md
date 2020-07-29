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
  
  // Initialize scheduler
  PriorityScheduler<threads<8>, priority_levels<5>> scheduler;
  scheduler.start();

  // Configure task
  Task t;
  t.on_execute([] {
    // execution time of task = 40ms
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
  });

  t.on_complete([](TaskStats stats) {
    std::cout << "Timer 1 fired! ";
    std::cout << "Wait time = " << stats.wait_time() << "ms; ";
    std::cout << "Computation time = " << stats.computation_time() << "ms; ";
    std::cout << "Response time = " << stats.response_time() << "ms\n";
  });

  // Schedule task periodically
  auto timer1 = std::thread([&scheduler, &t]() {
    do {
      // schedule task at priority level 3
      scheduler.schedule(t, 3);

      // sleep for 100ms
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while(true);
  });

  timer1.join();
}
```

* ***Wait time***: Duration of time for which the task is waiting in the queue.
* ***Computation time***: Duration of time taken for the task to execute once it's dequeued by a worker thread.
* ***Response time***: Duration of time taken to complete the task, once the task is marked as "ready". This is `wait_time + computation_time`.

```bash
▶ ./single_periodic_task
Timer 1 fired! Wait time = 0ms; Computation time = 42ms; Response time = 42ms
Timer 1 fired! Wait time = 0ms; Computation time = 42ms; Response time = 42ms
Timer 1 fired! Wait time = 0ms; Computation time = 44ms; Response time = 44ms
Timer 1 fired! Wait time = 0ms; Computation time = 43ms; Response time = 44ms
Timer 1 fired! Wait time = 0ms; Computation time = 43ms; Response time = 43ms
Timer 1 fired! Wait time = 0ms; Computation time = 43ms; Response time = 43ms
Timer 1 fired! Wait time = 0ms; Computation time = 40ms; Response time = 40ms
Timer 1 fired! Wait time = 0ms; Computation time = 43ms; Response time = 43ms
Timer 1 fired! Wait time = 0ms; Computation time = 40ms; Response time = 40ms
```

## Multiple Periodic Tasks

```cpp
#include <psched/priority_scheduler.h>
using namespace psched;

int main() {
  PriorityScheduler<threads<4>, priority_levels<5>> scheduler;
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
          std::cout << "[Lowest ] Priority Log Message ";
          std::cout << "Wait time = " << stats.wait_time() << "ms; ";
          std::cout << "Computation time = " << stats.computation_time() << "ms; ";
          std::cout << "Response time = " << stats.response_time() << "ms\n";
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
          std::cout << "[Low    ] Priority Log Message ";
          std::cout << "Wait time = " << stats.wait_time() << "ms; ";
          std::cout << "Computation time = " << stats.computation_time() << "ms; ";
          std::cout << "Response time = " << stats.response_time() << "ms\n";
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
          std::cout << "[Medium ] Priority Log Message ";
          std::cout << "Wait time = " << stats.wait_time() << "ms; ";
          std::cout << "Computation time = " << stats.computation_time() << "ms; ";
          std::cout << "Response time = " << stats.response_time() << "ms\n";
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
          std::cout << "[High   ] Priority Log Message ";
          std::cout << "Wait time = " << stats.wait_time() << "ms; ";
          std::cout << "Computation time = " << stats.computation_time() << "ms; ";
          std::cout << "Response time = " << stats.response_time() << "ms\n";
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
          std::cout << "[Highest] Priority Log Message ";
          std::cout << "Wait time = " << stats.wait_time() << "ms; ";
          std::cout << "Computation time = " << stats.computation_time() << "ms; ";
          std::cout << "Response time = " << stats.response_time() << "ms\n";
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
```

```bash
▶ ./multiple_periodic_tasks
[Lowest ] Priority Log Message Wait time = 0ms; Computation time = 40ms; Response time = 40ms
[Lowest ] Priority Log Message Wait time = 0ms; Computation time = 43ms; Response time = 43ms
[Medium ] Priority Log Message Wait time = 0ms; Computation time = 131ms; Response time = 131ms
[High   ] Priority Log Message Wait time = 0ms; Computation time = 393ms; Response time = 393ms
[Low    ] Priority Log Message Wait time = 0ms; Computation time = 82ms; Response time = 82ms
[Lowest ] Priority Log Message Wait time = 0ms; Computation time = 40ms; Response time = 40ms
[Lowest ] Priority Log Message Wait time = 0ms; Computation time = 42ms; Response time = 42ms
[Low    ] Priority Log Message Wait time = 0ms; Computation time = 84ms; Response time = 84ms
[Lowest ] Priority Log Message Wait time = 0ms; Computation time = 44ms; Response time = 44ms
[Low    ] Priority Log Message Wait time = 19ms; Computation time = 85ms; Response time = 104ms
[Medium ] Priority Log Message Wait time = 0ms; Computation time = 134ms; Response time = 134ms
[Low    ] Priority Log Message Wait time = 19ms; Computation time = 80ms; Response time = 99ms
[Lowest ] Priority Log Message Wait time = 154ms; Computation time = 40ms; Response time = 195ms
[Lowest ] Priority Log Message Wait time = 151ms; Computation time = 44ms; Response time = 196ms
[Low    ] Priority Log Message Wait time = 0ms; Computation time = 80ms; Response time = 80ms
[High   ] Priority Log Message Wait time = 0ms; Computation time = 392ms; Response time = 392ms
[Low    ] Priority Log Message Wait time = 0ms; Computation time = 83ms; Response time = 83ms
[Medium ] Priority Log Message Wait time = 33ms; Computation time = 130ms; Response time = 163ms
[Lowest ] Priority Log Message Wait time = 256ms; Computation time = 40ms; Response time = 297ms
[Lowest ] Priority Log Message Wait time = 259ms; Computation time = 40ms; Response time = 299ms
[Low    ] Priority Log Message Wait time = 0ms; Computation time = 82ms; Response time = 82ms
[Lowest ] Priority Log Message Wait time = 260ms; Computation time = 40ms; Response time = 301ms
[Highest] Priority Log Message Wait time = 0ms; Computation time = 561ms; Response time = 561ms
[Low    ] Priority Log Message Wait time = 29ms; Computation time = 82ms; Response time = 111ms
[Medium ] Priority Log Message Wait time = 0ms; Computation time = 134ms; Response time = 134ms
[Lowest ] Priority Log Message Wait time = 316ms; Computation time = 42ms; Response time = 359ms
[Low    ] Priority Log Message Wait time = 32ms; Computation time = 85ms; Response time = 117ms
[Lowest ] Priority Log Message Wait time = 370ms; Computation time = 42ms; Response time = 412ms
[Lowest ] Priority Log Message Wait time = 367ms; Computation time = 41ms; Response time = 409ms
[Low    ] Priority Log Message Wait time = 0ms; Computation time = 80ms; Response time = 80ms
[Lowest ] Priority Log Message Wait time = 364ms; Computation time = 41ms; Response time = 405ms
[Medium ] Priority Log Message Wait time = 0ms; Computation time = 133ms; Response time = 133ms
[High   ] Priority Log Message Wait time = 0ms; Computation time = 391ms; Response time = 391ms
```

## Generating Single Header

```bash
python3 utils/amalgamate/amalgamate.py -c single_include.json -s .
```

## Contributing
Contributions are welcome, have a look at the CONTRIBUTING.md document for more information.

## License
The project is available under the MIT license.
