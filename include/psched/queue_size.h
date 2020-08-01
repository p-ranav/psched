
#pragma once
#include <stddef.h>

namespace psched {

enum class discard { oldest_task, newest_task };

template <size_t queue_size, discard policy> struct maintain_size {
  constexpr static size_t bounded_queue_size = queue_size;
  constexpr static discard discard_policy = policy;
};

template <size_t count, class M = maintain_size<0, discard::oldest_task>> struct queues {
  constexpr static bool bounded_or_not = (M::bounded_queue_size > 0);
  constexpr static size_t number_of_queues = count;
  typedef M maintain_size;
};

} // namespace psched