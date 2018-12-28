#include <os>
#include <cstdio>

int main() {
  printf("Hello main - %s\n", OS::cmdline_args());
  assert(OS::cmdline_args() == std::string("service booted with vmrunner"));
  return 0;
}
