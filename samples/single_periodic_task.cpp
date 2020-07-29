#include <psched/priority_scheduler.h>
using namespace psched;

int main() {
  PriorityScheduler<threads<4>, priority_levels<5>> scheduler;
  scheduler.start();

  Task t;
  t.on_execute([] {
    // execution time of task = 40ms
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
  });

  t.on_complete([](TaskStats stats) {
    const auto computation_time = stats.computation_time<std::chrono::milliseconds>();
    const auto response_time = stats.response_time<std::chrono::milliseconds>();
    const auto wait_time = stats.wait_time<std::chrono::milliseconds>();
    std::cout << "Timer 1 fired! ";
    std::cout << "Wait time = " << wait_time << "ms; ";
    std::cout << "Computation time = " << computation_time << "ms; ";
    std::cout << "Response time = " << response_time << "ms\n";
  });

  // Periodic Timer 1 - 100ms period
  auto timer1 = std::thread([&scheduler, &t]() {
    while (true) {

      // schedule task at priority level 3
      scheduler.schedule(t, 3);

      // Sleep for 100ms
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  });

  timer1.join();
}