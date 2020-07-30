#include <psched/priority_scheduler.h>
#include <iostream>
using namespace psched;

/*
| Task | Period | Burst Time | Priority |
|------|--------|------------|----------|
| a    | 80     | 32         | 1        |
| b    | 40     |  5         | 2        |
| c    | 16     |  4         | 3        |
*/

int main() {
    // Initialize scheduler
    PriorityScheduler<threads<2>, priority_levels<3>> scheduler;
    scheduler.start();

    std::vector<TaskStats> a_stats, b_stats, c_stats;

    // Configure tasks
    {
        auto a = std::thread([&scheduler, &a_stats] {
          for (size_t i = 0; i < 100; i++) {
            Task t;
            t.on_execute([] { 
                std::this_thread::sleep_for(std::chrono::milliseconds(32)); 
            });
            t.on_complete([&a_stats](TaskStats stats) {
              a_stats.push_back(stats);
            });

            scheduler.schedule(t, 2);
            std::this_thread::sleep_for(std::chrono::milliseconds(80));
          }
        });

        auto b = std::thread([&scheduler, &b_stats] {
          for (size_t i = 0; i < 100; i++) {
            Task t;
            t.on_execute([] { 
            std::this_thread::sleep_for(std::chrono::milliseconds(5)); 
            });
            t.on_complete([&b_stats](TaskStats stats) {
              b_stats.push_back(stats);
            });

            scheduler.schedule(t, 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
          }
        });

        auto c = std::thread([&scheduler, &c_stats] {
          for (size_t i = 0; i < 100; i++) {
            Task t;
            t.on_execute([] { 
                std::this_thread::sleep_for(std::chrono::milliseconds(4)); 
            });
            t.on_complete([&c_stats](TaskStats stats) {
              c_stats.push_back(stats);
            });

            scheduler.schedule(t, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
          }
        });

        a.join();
        b.join();
        c.join();
    }

    // Tasks are done executing
    // Calculate average stats for completed tasks

    auto get_average_stats = [](const std::vector<TaskStats>& stats) -> std::tuple<long long, long long, long long> {
      if (stats.empty()) {
        return {0, 0, 0};
      }
      long long waiting_time_sum = 0, burst_time_sum = 0, turnaround_time_sum = 0;
      for (const auto& stat: stats) {
        waiting_time_sum += stat.waiting_time();
        burst_time_sum += stat.burst_time();
        turnaround_time_sum += stat.turnaround_time();
      }
      return {waiting_time_sum / stats.size(), burst_time_sum / stats.size(), turnaround_time_sum / stats.size()};
    };

    auto print_stats_tuple = [](const std::string & task_id, const std::tuple<long long, long long, long long> stats_avg) {
      std::cout << task_id << "; " 
                << std::get<0>(stats_avg) << "ms; "
                << std::get<1>(stats_avg) << "ms; "
                << std::get<2>(stats_avg) << "ms\n";
    };

    std::cout << "task_id; waiting_time; burst_time; turnaround_time\n";
    print_stats_tuple("a", get_average_stats(a_stats));
    print_stats_tuple("b", get_average_stats(b_stats));
    print_stats_tuple("c", get_average_stats(c_stats));

    scheduler.stop();
}