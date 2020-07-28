#include <chrono>
#include <iostream>
#include <pqsched/priority_scheduler.h>
#include <thread>
#include <random>
#include <functional>
using namespace pqsched;

int main() {

  std::random_device rd;  // obtain a random number from hardware
  std::mt19937 gen(rd()); // seed the generator
  std::uniform_int_distribution<> distr(0, 9); // define the range

  auto random_priority = [&]() { return distr(gen); };

  std::cout << "Scheduler Test 1\n";
  {
    PriorityScheduler<10> scheduler;

    // schedule task every 100ms
    auto t1 = std::thread([&]() {
      for (size_t i = 0; i < 500; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        const auto priority = random_priority();

        Task task;
        task.set_priority(priority);
        task.set_function( 
          [&]() {
            std::cout << "Executing task " << i << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); 
        });

        scheduler.schedule(task);
      }
    });

    // schedule task every 50ms
    auto t2 = std::thread([&]() {
      for (size_t i = 0; i < 500; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        const auto priority = random_priority();

        Task task;
        task.set_priority(priority);
        task.set_function( 
          [&]() {
            std::cout << "Executing task " << i << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); 
        });

        scheduler.schedule(task);

      }
    });

    t1.join();
    t2.join();
  }
}
