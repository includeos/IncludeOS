#include "common.hpp"

extern "C"
long syscall_n(long i) {
  STRACE("syscall_n, n=%i\n", i);
  return 0;
}
