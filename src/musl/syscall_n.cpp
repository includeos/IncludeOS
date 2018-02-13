#include "stub.hpp"

long syscall(long i){
  return 0;
};

extern "C"
long syscall_n(long i) {
  return stubtrace(syscall, "syscall", i);
}
