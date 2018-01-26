#include "common.hpp"

extern "C" void __serial_print(const char*, size_t);
extern "C"
long syscall_SYS_write(int fd, char* str, size_t len) {
  STUB("write");

  __serial_print(str, len);
  return 0;
}
