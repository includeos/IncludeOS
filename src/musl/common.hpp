#pragma once

#include <os>
#include <sstream>
#include <cstdio>

#define STUB(X) printf("<stubtrace> stubbed syscall %s  called\n", X)

#ifndef ENABLE_STRACE
#define ENABLE_STRACE false
#endif

constexpr bool __strace = ENABLE_STRACE;
extern "C" void __serial_print(const char*, size_t);

template <typename ...Args>
inline constexpr void pr_param([[maybe_unused]] std::ostream& out){

}

template <typename L, typename ...Args>
inline constexpr auto& pr_param(std::ostream& out,  L lhs, Args&&... rest){
  if constexpr (sizeof...(rest) > 0)
  {
    // avoid writing nullptr to std out
    if constexpr(std::is_pointer_v<L>) {
      if(lhs != nullptr) out << lhs << ", ";
      else out << "NULL, ";
    }
    else {
      out << lhs << ", ";
    }
    pr_param(out, rest...);
  }
  else
  {
    // avoid writing nullptr to std out
    if constexpr(std::is_pointer_v<L>) {
      if(lhs != nullptr) out << lhs;
      else out << "NULL";
    }
    else {
      out << lhs;
    }
  }
  return out;
}

template <typename Ret, typename ...Args>
inline void strace_print(const char* name, Ret ret, Args&&... args){
  extern bool __libc_initialized;

  if (not __libc_initialized)
    return;

  std::stringstream out;
  out << name << "(";
  pr_param(out, args...);
  out << ") = " << ret;
  //if (errno)
  //  out << " " << strerror(errno);
  out << '\n';
  auto str = out.str();
  __serial_print(str.data(), str.size());
}

// strace, calling the syscall, recording return value and printing if enabled
template<typename Fn, typename ...Args>
inline auto strace(Fn func, const char* name, Args&&... args) {
  auto ret = func(args...);

  if constexpr (__strace)
     strace_print(name, ret, args...);
  (void) name;

  return ret;
}
