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
        task.on_execute( 
          [&]() {
            // do work here
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
        });

        task.on_complete(
          [&](TaskStats stats) {
            const auto computation_time = stats.computation_time<std::chrono::milliseconds>();
            const auto response_time = stats.response_time<std::chrono::milliseconds>();
            std::cout << "Executed task " << i 
                      << ". Response time = " << response_time 
                      << " milliseconds, Computation time = " 
                      << computation_time << " milliseconds\n";
          }
        );

        task.on_error(
          [](TaskStats stats, const char * error) {
            std::cout << error << "\n";
          }
        );

        scheduler.schedule(task);
      }
    });

    // // schedule task every 50ms
    // auto t2 = std::thread([&]() {
    //   for (size_t j = 0; j < 500; ++j) {
    //     std::this_thread::sleep_for(std::chrono::milliseconds(50));
    //     const auto priority = random_priority();

    //     Task task;
    //     task.set_priority(priority);
    //     task.on_execute( 
    //       [&]() {
    //         std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
    //     });
    //     task.on_complete(
    //       [&](TaskStats stats) {
    //         // const auto computation_time = stats.computation_time<std::chrono::milliseconds>();
    //         // const auto response_time = stats.response_time<std::chrono::milliseconds>();
    //         // std::cout << "Executed task " << j
    //         //           << ". Response time = " << response_time 
    //         //           << " milliseconds, Computation time = " 
    //         //           << computation_time << " milliseconds\n";
    //       }
    //     );
    //     task.on_error(
    //       [](TaskStats stats, const char * error) {
    //         std::cout << error << "\n";
    //       }
    //     );

    //     scheduler.schedule(task);

    //   }
    // });

    // // schedule task every 200ms
    // auto t3 = std::thread([&]() {
    //   for (size_t k = 0; k < 500; ++k) {
    //     std::this_thread::sleep_for(std::chrono::milliseconds(200));
    //     const auto priority = random_priority();

    //     Task task;
    //     task.set_priority(priority);
    //     task.on_execute( 
    //       [&]() {
    //         std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
    //     });
    //     task.on_complete(
    //       [&](TaskStats stats) {
    //         // const auto computation_time = stats.computation_time<std::chrono::milliseconds>();
    //         // const auto response_time = stats.response_time<std::chrono::milliseconds>();
    //         // std::cout << "Executed task " << k
    //         //           << ". Response time = " << response_time 
    //         //           << " milliseconds, Computation time = " 
    //         //           << computation_time << " milliseconds\n";
    //       }
    //     );
    //     task.on_error(
    //       [](TaskStats stats, const char * error) {
    //         std::cout << error << "\n";
    //       }
    //     );

    //     scheduler.schedule(task);

    //   }
    // });

    t1.join();
    // t2.join();
    // t3.join();
  }
}
