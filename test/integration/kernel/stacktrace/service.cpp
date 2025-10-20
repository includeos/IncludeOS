#include <os>

void Service::start()
{
  printf("Calling os::print_backtrace()\n");
  os::print_backtrace();
  printf("We reached the end.\n");
}
