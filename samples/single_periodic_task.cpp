#include <psched/priority_scheduler.h>
using namespace psched;

int main() {
  
  // Initialize scheduler
  PriorityScheduler<threads<8>, priority_levels<5>> scheduler;
  scheduler.start();

  // Configure task
  Task t;
  t.on_execute([] {
    // execution time of task = 40ms
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
  });

  t.on_complete([](TaskStats stats) {
    std::cout << "Timer 1 fired! ";
    std::cout << "Wait time = " << stats.wait_time() << "ms; ";
    std::cout << "Computation time = " << stats.computation_time() << "ms; ";
    std::cout << "Response time = " << stats.response_time() << "ms\n";
  });

  // Schedule task periodically
  auto timer1 = std::thread([&scheduler, &t]() {
    while (true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      scheduler.schedule(t, 3); // schedule at priority level 3
    }
  });

  timer1.join();
}