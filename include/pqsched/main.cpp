#include <pqsched/scheduler.h>
#include <iostream>
#include <chrono>
#include <thread>
using namespace std::chrono_literals;

#include <random>

int main() {

  std::random_device rd; // obtain a random number from hardware
  std::mt19937 gen(rd()); // seed the generator
  std::uniform_int_distribution<> distr(0, 4); // define the range

  auto random_priority = [&]() { return distr(gen); };
  
  {    
    pqsched::scheduler<int, 5> scheduler;
    for (size_t i = 0; i < 5; ++i) {     
      std::this_thread::sleep_for(2s);
      const auto priority = random_priority();
      std::cout << "Enqueuing task " << i << " in queue with priority " << priority << std::endl;
      scheduler.schedule(i, priority);
    }
    
  }
}
