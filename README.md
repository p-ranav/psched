# psched

`psched` is a lightweight library that provides a priority-based task scheduler for modern C++.

## Getting Started

To get started, create a `PriorityScheduler`

```cpp
#include <psched/priority_scheduler.h>
using namespace psched;

int main() {
  PriorityScheduler<<threads<4>, priority_levels<5>> scheduler;
}
```

<p align="center">
  <img height="400" src="img/priority_scheduling.png"/>  
</p>

## Generating Single Header

```bash
python3 utils/amalgamate/amalgamate.py -c single_include.json -s .
```

## Contributing
Contributions are welcome, have a look at the CONTRIBUTING.md document for more information.

## License
The project is available under the MIT license.
