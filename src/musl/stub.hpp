#include "common.hpp"

template<typename R, typename ...Args>
inline void stubtrace_print(const char* name, R ret, Args&&... args) {
  printf("<WARNING> Stubbed syscall: ");
  strace_print(name, ret, args...);
}

// Strace for stubbed syscalls.
// calling the syscall, recording return value and always printing
template<typename Fn, typename ...Args>
inline auto stubtrace(Fn func, const char* name, Args&&... args) {
  auto ret = func(args...);
  stubtrace_print(name, ret, args...);
  return ret;
}
