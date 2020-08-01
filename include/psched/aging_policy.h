
#pragma once
#include <chrono>

namespace psched {

template <typename T> struct is_chrono_duration { static constexpr bool value = false; };

template <typename Rep, typename Period>
struct is_chrono_duration<std::chrono::duration<Rep, Period>> {
  static constexpr bool value = true;
};

template <class D = std::chrono::milliseconds, size_t P = 0> struct task_starvation_after {
  static_assert(is_chrono_duration<D>::value, "Duration must be a std::chrono::duration");
  typedef D type;
  constexpr static D value = D(P);
};

template <size_t P> struct increment_priority_by { constexpr static size_t value = P; };

template <class T = task_starvation_after<>, class I = increment_priority_by<1>>
struct aging_policy {
  typedef T task_starvation_after;
  typedef I increment_priority_by;
};

} // namespace psched