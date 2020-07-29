#include <psched/priority_scheduler.h>
using namespace psched;

int main() {
  PriorityScheduler<threads<5>, priority_levels<2>> scheduler;
  scheduler.start();

  {
      // High-frequency low-priority task
      auto thread1 = std::thread(
        [&scheduler]() {
          while(true) {
            Task t1;
            t1.set_id(0);
            t1.set_priority(1);
            t1.on_execute([]{
              // task takes 10ms to execute
              std::this_thread::sleep_for(std::chrono::milliseconds(2));
            });
            t1.on_complete([](TaskStats stats) {
                const auto computation_time = stats.computation_time<std::chrono::milliseconds>();
                const auto response_time = stats.response_time<std::chrono::milliseconds>();
                std::cout << "Executed task [" << stats.task_id << "] Priority = " << stats.task_priority
                        << ". Response time = " << response_time 
                        << " milliseconds, Computation time = " 
                        << computation_time << " milliseconds\n";
            });

            scheduler.schedule(t1);

            // task period = 100ms
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
          }
        }  
      );

      // Low-frequency high-priority task
      auto thread2 = std::thread(
        [&scheduler]() {
          while(true) {
            Task t2;
            t2.set_id(1);
            t2.set_priority(0);
            t2.on_execute([]{
                // task takes 500ms to execute
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            });
            t2.on_complete([](TaskStats stats) {
                const auto computation_time = stats.computation_time<std::chrono::milliseconds>();
                const auto response_time = stats.response_time<std::chrono::milliseconds>();
                std::cout << "Executed task [" << stats.task_id << "] Priority = " << stats.task_priority
                        << ". Response time = " << response_time 
                        << " milliseconds, Computation time = " 
                        << computation_time << " milliseconds\n";
            });
            scheduler.schedule(t2);

            // task period = 1s
            std::this_thread::sleep_for(std::chrono::seconds(1));
          }
        }
      );

      thread1.join();
      thread2.join();
  }

}