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
  PriorityScheduler<threads<5>, priority_levels<10>> scheduler;
  scheduler.start();

  {

    // schedule task every 100ms
    auto t1 = std::thread([&]() {
      for (size_t i = 0; i < 5; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        const auto priority = random_priority();

        Task task;
        task.set_id(i);
        task.set_priority(priority);
        task.on_execute( 
          [&]() {
            // do work here
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
        });

        task.on_complete(
          [](TaskStats stats) {
            const auto computation_time = stats.computation_time<std::chrono::milliseconds>();
            const auto response_time = stats.response_time<std::chrono::milliseconds>();
            std::cout << "Executed task [" << stats.task_id << "] Priority = " << stats.task_priority
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

    // schedule task every 50ms
    auto t2 = std::thread([&]() {
      for (size_t i = 0; i < 500; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        const auto priority = random_priority();

        Task task;
        task.set_id(i);
        task.set_priority(priority);
        task.on_execute( 
          [&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
        });
        task.on_complete(
          [](TaskStats stats) {
            const auto computation_time = stats.computation_time<std::chrono::milliseconds>();
            const auto response_time = stats.response_time<std::chrono::milliseconds>();
            std::cout << "Executed task [" << stats.task_id << "] Priority = " << stats.task_priority
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

    // schedule task every 200ms
    auto t3 = std::thread([&]() {
      for (size_t i = 0; i < 140; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        const auto priority = random_priority();

        Task task;
        task.set_id(i);
        task.set_priority(priority);
        task.on_execute( 
          [&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
        });
        task.on_complete(
          [](TaskStats stats) {
            const auto computation_time = stats.computation_time<std::chrono::milliseconds>();
            const auto response_time = stats.response_time<std::chrono::milliseconds>();
            std::cout << "Executed task [" << stats.task_id << "] Priority = " << stats.task_priority
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

    t1.join();
    t2.join();
    t3.join();
  }

  std::this_thread::sleep_for(std::chrono::seconds(5));
  std::cout << "All tasks completed\n";
}