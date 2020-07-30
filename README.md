# psched

`psched` is a lightweight library that provides a priority-based task scheduler for modern C++.

* The scheduler manages an array of concurrent queues, each queue assigned a priority-level
* A task, when scheduled, is enqueued onto one of queues based on the task's priority
* A pool of threads executes ready tasks, starting with the highest priority

<p align="center">
  <img height="300" src="img/priority_scheduling.png"/>  
</p>

## Samples

### Single Sporadic Task 

The following example:
* creates a `PriorityScheduler` object with 1 worker thread and 1 priority level.
* configures a `Task` which, when executed, will print `"Hello World"`.
* schedules the task for execution.

```cpp
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
```

* The above task is sporadic and executed once:

```bash
./single_sporadic_task
Hello World
```

### Single Periodic Task
  
| Task | Period (ms) | Burst Time (ms) | Priority   |
|------|-------------|-----------------|------------|
| a    | 100         | 40              | 0 (Lowest) |

```cpp
#include <iostream>
#include <psched/priority_scheduler.h>
using namespace psched;

int main() {

  // Initialize scheduler
  PriorityScheduler<threads<2>, priority_levels<1>> scheduler;

  // Configure task
  Task task;

  task.on_execute([] {
    // execution time (burst time) of task = 40ms
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
  });

  task.on_complete([](const TaskStats& stats) {
    std::cout << "Timer 1 fired! ";
    std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
    std::cout << "Burst time = " << stats.burst_time() << "ms; ";
    std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
  });

  // Schedule periodic task
  scheduler.schedule<priority<0>, 
                     period<std::chrono::milliseconds, 100>>(task);
}
```

Note the use of `task.on_complete`. When a task is complete, it's `on_complete` function (if one is provided) will be called. `TaskStats` can be used to study the temporal behavior of the task. This includes:
* ***Waiting time***: Duration of time for which the task is waiting in the queue.
* ***Burst time***: Duration of time taken for the task to execute once it's dequeued by a worker thread.
* ***Turnaround time***: Duration of time taken to complete the task, once the task is marked as "ready". 
  - `turnaround_time = waiting_time + burst_time`

```bash
▶ ./single_periodic_task
Timer 1 fired! Waiting time = 0ms; Burst time = 42ms; Turnaround time = 42ms
Timer 1 fired! Waiting time = 0ms; Burst time = 40ms; Turnaround time = 40ms
Timer 1 fired! Waiting time = 0ms; Burst time = 44ms; Turnaround time = 44ms
Timer 1 fired! Waiting time = 0ms; Burst time = 40ms; Turnaround time = 40ms
Timer 1 fired! Waiting time = 0ms; Burst time = 41ms; Turnaround time = 41ms
Timer 1 fired! Waiting time = 0ms; Burst time = 43ms; Turnaround time = 43ms
Timer 1 fired! Waiting time = 0ms; Burst time = 40ms; Turnaround time = 40ms
Timer 1 fired! Waiting time = 0ms; Burst time = 43ms; Turnaround time = 43ms
Timer 1 fired! Waiting time = 0ms; Burst time = 44ms; Turnaround time = 44ms
```

### Multiple Periodic Tasks

| Task | Period (ms) | Burst Time (ms) | Priority    |
|------|-------------|-----------------|-------------|
| a    |  250        | 130             | 0 (Lowest)  |
| b    |  500        | 390             | 1           |
| c    | 1000        | 560             | 2 (Highest) |

```cpp
#include <iostream>
#include <psched/priority_scheduler.h>
using namespace psched;

int main() {
  PriorityScheduler<threads<2>, priority_levels<3>> scheduler;

  Task a
  (
    // Task action
    [] {
      std::this_thread::sleep_for(std::chrono::milliseconds(130));
    },
    // Task post-completion callback
    [](const TaskStats& stats) {
      std::cout << "[Task a] ";
      std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
      std::cout << "Burst time = " << stats.burst_time() << "ms; ";
      std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
    }
  );

  Task b
  (
    // Task action
    [] {
      std::this_thread::sleep_for(std::chrono::milliseconds(390));
    },
    // Task post-completion callback
    [](const TaskStats& stats) {
      std::cout << "[Task b] ";
      std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
      std::cout << "Burst time = " << stats.burst_time() << "ms; ";
      std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
    }
  );

  Task c
  (
    // Task action
    [] {
      std::this_thread::sleep_for(std::chrono::milliseconds(560));
    },
    // Task post-completion callback
    [](const TaskStats& stats) {
      std::cout << "[Task c] ";
      std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
      std::cout << "Burst time = " << stats.burst_time() << "ms; ";
      std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
    }
  );

  // Schedule the tasks
  scheduler.schedule<priority<0>, period<std::chrono::milliseconds, 250>>(a);
  scheduler.schedule<priority<1>, period<std::chrono::milliseconds, 500>>(b);
  scheduler.schedule<priority<2>, period<std::chrono::milliseconds, 1000>>(c);
}
```

* Note that Task `a`, which is the lowest priority task, waits in the queue for longer than Tasks `b` and `c`.

```bash
▶ ./multiple_periodic_tasks
[Task a] Waiting time = 0ms; Burst time = 131ms; Turnaround time = 131ms
[Task a] Waiting time = 0ms; Burst time = 131ms; Turnaround time = 131ms
[Task a] Waiting time = 0ms; Burst time = 135ms; Turnaround time = 135ms
[Task b] Waiting time = 0ms; Burst time = 394ms; Turnaround time = 394ms
[Task a] Waiting time = 253ms; Burst time = 134ms; Turnaround time = 388ms
[Task b] Waiting time = 0ms; Burst time = 393ms; Turnaround time = 393ms
[Task c] Waiting time = 0ms; Burst time = 563ms; Turnaround time = 563ms
[Task a] Waiting time = 255ms; Burst time = 132ms; Turnaround time = 387ms
[Task b] Waiting time = 254ms; Burst time = 133ms; Turnaround time = 387ms
[Task a] Waiting time = 0ms; Burst time = 393ms; Turnaround time = 393ms
[Task a] Waiting time = 252ms; Burst time = 130ms; Turnaround time = 382ms
[Task b] Waiting time = 0ms; Burst time = 392ms; Turnaround time = 392ms
[Task a] Waiting time = 250ms; Burst time = 130ms; Turnaround time = 380ms
[Task c] Waiting time = 0ms; Burst time = 561ms; Turnaround time = 561ms
[Task a] Waiting time = 251ms; Burst time = 131ms; Turnaround time = 382ms
[Task b] Waiting time = 0ms; Burst time = 393ms; Turnaround time = 393ms
[Task a] Waiting time = 250ms; Burst time = 130ms; Turnaround time = 380ms
[Task a] Waiting time = 255ms; Burst time = 130ms; Turnaround time = 385ms
[Task b] Waiting time = 0ms; Burst time = 391ms; Turnaround time = 391ms
[Task a] Waiting time = 250ms; Burst time = 133ms; Turnaround time = 384ms
[Task c] Waiting time = 0ms; Burst time = 564ms; Turnaround time = 564ms
[Task a] Waiting time = 250ms; Burst time = 133ms; Turnaround time = 383ms
[Task b] Waiting time = 0ms; Burst time = 392ms; Turnaround time = 392ms
[Task a] Waiting time = 252ms; Burst time = 131ms; Turnaround time = 383ms
```

## Generating Single Header

```bash
python3 utils/amalgamate/amalgamate.py -c single_include.json -s .
```

## Contributing
Contributions are welcome, have a look at the CONTRIBUTING.md document for more information.

## License
The project is available under the MIT license.
