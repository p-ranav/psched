# psched

`psched` is a lightweight library that provides a priority-based task scheduler for modern C++.

* The `psched` scheduler manages an array of concurrent queues, each queue assigned a priority-level
* A task, when scheduled, is enqueued onto one of queues based on the task's priority
* A pool of threads executes ready tasks, starting with the highest priority
* The priority of starving tasks is modulated based on the age of the task

<p align="center">
  <img height="400" src="img/priority_scheduling.png"/>  
</p>

## Getting Started

Consider the task set below. There are three periodic tasks: `a`, `b` and `c`. 

| Task | Period (ms) | Burst Time (ms) | Priority    |
|------|-------------|-----------------|-------------|
| a    |  250        | 130             | 0 (Lowest)  |
| b    |  500        | 390             | 1           |
| c    | 1000        | 560             | 2 (Highest) |

Here, _burst time_ refers to the amount of time required by the task for executing on CPU.

First, let's create a scheduler:

```cpp
#include <iostream>
#include <psched/priority_scheduler.h>
using namespace psched;

int main() {
  PriorityScheduler<threads<3>, 
                    priority_levels<3>, 
                    task_starvation<std::chrono::milliseconds, 2500>> scheduler;
```

* Specify the number of worker threads using `threads<size_t>`
* Specify the number of priority levels, i.e., the number of queues/lanes, using `priority_levels<size_t>`
* Specify the criteria for task starvation using `task_starvation<duration>`
  - In priority-based scheduling, Each task is assigned a priority and the process with the highest priority is executed first. A steady flow of CPU bursts from the high priority processes can starve the low-priority ones. To solve this problem, age-based priority modulation is used. Here, specifying `task_starvation<std::chrono::milliseconds, 2500>>` specifies that any task at a lower priority that is starved of the CPU, i.e., waiting in a queue, for more than `2.5s` will get a bump in priority.

## Samples

### Single Sporadic Task 

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
  PriorityScheduler<threads<4>, priority_levels<1>> scheduler;

  // Configure task
  Task task(
      // Task action
      [] { std::this_thread::sleep_for(std::chrono::milliseconds(40)); },
      // Task post-completion callback
      [](const TaskStats &stats) {
        std::cout << "Timer 1 fired! ";
        std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
        std::cout << "Burst time = " << stats.burst_time() << "ms; ";
        std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
      });

  // Schedule periodic task
  auto periodic_timer = std::thread([&scheduler, &task]() {
    while (true) {
      scheduler.schedule<priority<0>>(task);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  });
  periodic_timer.join();
}
```

Note the use of `task.on_complete`. When a task is complete, it's `on_complete` function (if one is provided) will be called. `TaskStats` can be used to study the temporal behavior of the task. This includes:
* ***Waiting time***: Duration of time for which the task is waiting in the queue.
* ***Burst time***: Duration of time taken for the task to execute once it's dequeued by a worker thread.
* ***Turnaround time***: Duration of time taken to complete the task, once the task is marked as "ready". 
  - `turnaround_time = waiting_time + burst_time`

```bash
./single_periodic_task
Timer 1 fired! Waiting time = 0ms; Burst time = 40ms; Turnaround time = 40ms
Timer 1 fired! Waiting time = 0ms; Burst time = 40ms; Turnaround time = 40ms
Timer 1 fired! Waiting time = 0ms; Burst time = 42ms; Turnaround time = 42ms
Timer 1 fired! Waiting time = 0ms; Burst time = 44ms; Turnaround time = 44ms
Timer 1 fired! Waiting time = 0ms; Burst time = 43ms; Turnaround time = 43ms
Timer 1 fired! Waiting time = 0ms; Burst time = 43ms; Turnaround time = 43ms
Timer 1 fired! Waiting time = 0ms; Burst time = 44ms; Turnaround time = 44ms
Timer 1 fired! Waiting time = 0ms; Burst time = 40ms; Turnaround time = 40ms
Timer 1 fired! Waiting time = 0ms; Burst time = 40ms; Turnaround time = 40ms
Timer 1 fired! Waiting time = 0ms; Burst time = 40ms; Turnaround time = 40ms
Timer 1 fired! Waiting time = 0ms; Burst time = 45ms; Turnaround time = 45ms
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
  PriorityScheduler<threads<3>, 
                    priority_levels<3>, 
                    task_starvation<std::chrono::milliseconds, 250>> scheduler;

  Task a(
      // Task action
      [] { std::this_thread::sleep_for(std::chrono::milliseconds(130)); },
      // Task post-completion callback
      [](const TaskStats &stats) {
        std::cout << "[Task a] ";
        std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
        std::cout << "Burst time = " << stats.burst_time() << "ms; ";
        std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
      });

  auto timer_a = std::thread([&scheduler, &a]() {
    while (true) {
      scheduler.schedule<priority<0>>(a);
      std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
  });

  Task b(
      // Task action
      [] { std::this_thread::sleep_for(std::chrono::milliseconds(390)); },
      // Task post-completion callback
      [](const TaskStats &stats) {
        std::cout << "[Task b] ";
        std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
        std::cout << "Burst time = " << stats.burst_time() << "ms; ";
        std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
      });

  auto timer_b = std::thread([&scheduler, &b]() {
    while (true) {
      scheduler.schedule<priority<1>>(b);
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  });

  Task c(
      // Task action
      [] { std::this_thread::sleep_for(std::chrono::milliseconds(560)); },
      // Task post-completion callback
      [](const TaskStats &stats) {
        std::cout << "[Task c] ";
        std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
        std::cout << "Burst time = " << stats.burst_time() << "ms; ";
        std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
      });

  auto timer_c = std::thread([&scheduler, &c]() {
    while (true) {
      scheduler.schedule<priority<2>>(c);
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
  });

  timer_a.join();
  timer_b.join();
  timer_c.join();
}
```

* Note that Task `a`, which is the lowest priority task, waits in the queue for longer than Tasks `b` and `c`.

```bash
./multiple_periodic_tasks
[Task a] Waiting time = 0ms; Burst time = 133ms; Turnaround time = 134ms
[Task a] Waiting time = 0ms; Burst time = 135ms; Turnaround time = 135ms
[Task b] Waiting time = 0ms; Burst time = 394ms; Turnaround time = 394ms
[Task c] Waiting time = 0ms; Burst time = 560ms; Turnaround time = 560ms
[Task a] Waiting time = 0ms; Burst time = 135ms; Turnaround time = 135ms
[Task b] Waiting time = 0ms; Burst time = 392ms; Turnaround time = 392ms
[Task a] Waiting time = 0ms; Burst time = 132ms; Turnaround time = 132ms
[Task a] Waiting time = 0ms; Burst time = 132ms; Turnaround time = 132ms
[Task b] Waiting time = 0ms; Burst time = 393ms; Turnaround time = 393ms
[Task a] Waiting time = 0ms; Burst time = 132ms; Turnaround time = 132ms
[Task c] Waiting time = 0ms; Burst time = 564ms; Turnaround time = 564ms
[Task a] Waiting time = 0ms; Burst time = 134ms; Turnaround time = 134ms
[Task b] Waiting time = 0ms; Burst time = 390ms; Turnaround time = 390ms
[Task a] Waiting time = 0ms; Burst time = 134ms; Turnaround time = 134ms
[Task a] Waiting time = 0ms; Burst time = 134ms; Turnaround time = 134ms
[Task b] Waiting time = 0ms; Burst time = 391ms; Turnaround time = 391ms
```

### Catching Exceptions

```cpp
#include <iostream>
#include <psched/priority_scheduler.h>
using namespace psched;

int main() {
  // Initialize the scheduler
  PriorityScheduler<threads<1>, priority_levels<1>> scheduler;

  // Configure the task
  Task fail(
      // Task action
      [] { throw std::runtime_error("Task Error: Uh oh, something bad happened"); },

      // Task post-completion callback
      {},

      // Task error callback
      [](const TaskStats &stats, const char *error_message) {
        std::cout << error_message << "\n";
      });

  // Schedule the task
  scheduler.schedule<priority<0>>(fail);
}
```

```bash
./task_error
Task Error: Uh oh, something bad happened
```

## Generating Single Header

```bash
python3 utils/amalgamate/amalgamate.py -c single_include.json -s .
```

## Contributing
Contributions are welcome, have a look at the CONTRIBUTING.md document for more information.

## License
The project is available under the MIT license.
