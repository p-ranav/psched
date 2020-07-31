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
  PriorityScheduler<threads<4>, 
                    priority_levels<3>, 
                    task_starvation<std::chrono::milliseconds, 250>> scheduler;
```

* Specify the number of worker threads using `threads<size_t>`
* Specify the number of priority levels, i.e., the number of queues/lanes, using `priority_levels<size_t>`
* Specify the criteria for task starvation using `task_starvation<duration>`
  - In priority-based scheduling, Each task is assigned a priority and the process with the highest priority is executed first. A steady flow of CPU bursts from the high priority processes can starve the low-priority ones. To solve this problem, age-based priority modulation is used. Here, specifying `task_starvation<std::chrono::milliseconds, 250>>` specifies that any task at a lower priority that is starved of the CPU, i.e., waiting in a queue, for more than `250ms` will get a bump in priority.
  
 Create the first task, `Task a` like below. The task "performs work" for 130ms. The post-completion callback can be used to study the temporal behavior of the task, e.g., waiting time, burst time, turnaround time etc.
 
 ```cpp
 
  Task a(
      // Task action
      [] { std::this_thread::sleep_for(std::chrono::milliseconds(130)); },
  
      // Task post-completion callback
      [](const TaskStats &stats) {
        std::cout << "[Task a] ";
        std::cout << "Waiting time = " << stats.waiting_time() << "ms; ";
        std::cout << "Burst time = " << stats.burst_time() << "ms; ";
        std::cout << "Turnaround time = " << stats.turnaround_time() << "ms\n";
      }
  );
 ```
 
Now, we can write a simple periodic timer that schedules this task at `priority<0>`:

```cpp
  auto timer_a = std::thread([&scheduler, &a]() {
    while (true) {
    
      // Schedule the task
      scheduler.schedule<priority<0>>(a);
      
      // Sleep for 250ms and repeat
      std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
  });
```

We can repeat the above code for tasks `b` and `c`:

```cpp
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

Running this sample may yield the following output:

```bash
./multiple_periodic_tasks
[Task a] Waiting time = 0ms; Burst time = 133ms; Turnaround time = 133ms
[Task c] Waiting time = 0ms; Burst time = 563ms; Turnaround time = 563ms
[Task b] Waiting time = 0ms; Burst time = 395ms; Turnaround time = 395ms
[Task a] Waiting time = 60ms; Burst time = 134ms; Turnaround time = 194ms
[Task a] Waiting time = 0ms; Burst time = 131ms; Turnaround time = 131ms
[Task b] Waiting time = 0ms; Burst time = 390ms; Turnaround time = 390ms
[Task a] Waiting time = 3ms; Burst time = 135ms; Turnaround time = 139ms
[Task a] Waiting time = 0ms; Burst time = 132ms; Turnaround time = 132ms
[Task b] Waiting time = 8ms; Burst time = 393ms; Turnaround time = 402ms
[Task c] Waiting time = 0ms; Burst time = 561ms; Turnaround time = 561ms
[Task a] Waiting time = 6ms; Burst time = 133ms; Turnaround time = 139ms
[Task b] Waiting time = [Task a] Waiting time = 0ms; Burst time = 393ms; Turnaround time = 393ms
0ms; Burst time = 133ms; Turnaround time = 133ms
[Task a] Waiting time = 11ms; Burst time = 134ms; Turnaround time = 145ms
[Task a] Waiting time = 0ms; Burst time = 134ms; Turnaround time = 134ms
[Task b] Waiting time = 11ms; Burst time = 394ms; Turnaround time = 405ms
[Task c] Waiting time = 0ms; Burst time = 560ms; Turnaround time = 560ms
[Task a] Waiting time = 7ms; Burst time = 132ms; Turnaround time = 139ms
[Task b] Waiting time = 0ms; Burst time = 390ms; Turnaround time = 390ms
[Task a] Waiting time = 0ms; Burst time = 130ms; Turnaround time = 130ms
[Task a] Waiting time = 17ms; Burst time = 130ms; Turnaround time = 148ms
[Task a] Waiting time = 0ms; Burst time = 131ms; Turnaround time = 131ms
[Task b] Waiting time = 10ms; Burst time = 390ms; Turnaround time = 401ms
[Task c] Waiting time = 0ms; Burst time = 560ms; Turnaround time = 560ms
```

## Generating Single Header

```bash
python3 utils/amalgamate/amalgamate.py -c single_include.json -s .
```

## Contributing
Contributions are welcome, have a look at the CONTRIBUTING.md document for more information.

## License
The project is available under the MIT license.
