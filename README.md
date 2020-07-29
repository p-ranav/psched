# psched

`psched` is a lightweight library that provides a priority-based task scheduler for modern C++.

## Getting Started

* The priority scheduler manages an array of concurrent queues, each queue assigned a priority-level
* A task, when scheduled, is enqueued onto one of queues based on the task's priority
* A pool of threads executes ready tasks, starting with the highest priority

<p align="center">
  <img height="400" src="img/priority_scheduling.png"/>  
</p>

Create a `PriorityScheduler` like below:

```cpp
#include <psched/priority_scheduler.h>
using namespace psched;

int main() {
  PriorityScheduler<<threads<4>, priority_levels<5>> scheduler;
}
```

* The above scheduler uses 4 worker threads and has 5 queues (for the 5 priority levels). 
  - Priority 0 is the highest priority
  - Priority 4 is the lowest priority

To schedule a task, create a `psched::Task`:

```cpp
Task t;
t.set_priority(4); // lowest priority

t.on_execute([] {
    // execution time of task = 40ms
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
});

t.on_complete([] (TaskStats stats) {
    const auto computation_time = stats.computation_time<std::chrono::milliseconds>();
    const auto response_time = stats.response_time<std::chrono::milliseconds>();
    const auto wait_time = stats.wait_time<std::chrono::milliseconds>();
    
    std::cout << "Executed task [" << stats.task_id << "] Priority = " << stats.task_priority
            << ", Response time = " << response_time 
            << " milliseconds, Computation time = " 
            << computation_time << " milliseconds\n";
});

scheduler.schedule(t);
```

## Generating Single Header

```bash
python3 utils/amalgamate/amalgamate.py -c single_include.json -s .
```

## Contributing
Contributions are welcome, have a look at the CONTRIBUTING.md document for more information.

## License
The project is available under the MIT license.
