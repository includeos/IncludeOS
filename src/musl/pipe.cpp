#include "common.hpp"

extern "C" {
long syscall_SYS_pipe() {
  STUB("pipe");
  return 0;
}

long syscall_SYS_pipe2() {
  STUB("pipe2");
  return 0;
}
} // extern "C"
