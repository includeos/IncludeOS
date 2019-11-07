
#include <os>
#include <timers>
using namespace std::chrono;

extern "C" uint32_t os_get_blocking_level();
extern "C" uint32_t os_get_highest_blocking_level();

static void msleep(int micros)
{
  bool ticked = false;
  Timers::oneshot(microseconds(micros),
  [&ticked] (int) {
    INFO("Timer", "Ticked");
    ticked = true;
    Expects(os_get_blocking_level() == 1);
  });

  while (ticked == false) {
    os::block();
    Expects(os_get_blocking_level() == 0);
  }
}

void Service::start()
{
  INFO("Block", "Testing blocking calls.");
}

void Service::ready()
{
  static int sleeps = 0;

  int n = 10;
  for (int i = 0; i < n; i++) {
    msleep(1000000);
    sleeps++;
    printf("Sleep %i/%i \n", sleeps, n);
    if (sleeps == 10)
    INFO("Test", "SUCCESS");
  }

  Expects(os_get_blocking_level() == 0);
  Expects(os_get_highest_blocking_level() == 1);
}
