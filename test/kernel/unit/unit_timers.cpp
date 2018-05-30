#include <common.cxx>
#include <kernel/timers.hpp>
using namespace std::chrono;

extern delegate<uint64_t()> systime_override;
static uint64_t current_time = 0;

static int magic_performed = 0;
void perform_magic(int) {
  magic_performed += 1;
}

CASE("Initialize timer system")
{
  systime_override =
    [] () -> uint64_t { return current_time; };

  Timers::init(
    [] (Timers::duration_t) {},
    [] () {}
  );
  Timers::ready();

  EXPECT(Timers::is_ready());
  EXPECT(Timers::active() == 0);
  EXPECT(Timers::existing() == 0);
  EXPECT(Timers::free() == 0);
}

CASE("Start a single timer, execute it")
{
  current_time = 0;
  magic_performed = 0;
  // start timer
  Timers::oneshot(1ms, perform_magic);
  EXPECT(Timers::active() == 1);
  EXPECT(Timers::existing() == 1);
  EXPECT(Timers::free() == 0);
  // execute timer interrupt
  Timers::timers_handler();
  // verify timer did not execute
  EXPECT(magic_performed == 0);
  // set time to where it should happen
  current_time = 1000000;
  // execute timer interrupt
  Timers::timers_handler();
  // verify timer did execute
  EXPECT(magic_performed == 1);
}

CASE("Start a single timer, execute it precisely")
{
  current_time = 0;
  magic_performed = 0;
  // start timer
  Timers::oneshot(1ms, perform_magic);
  // verify timer did not execute for all times before 1ms
  for (uint64_t time = 0; time < 1000000; time += 1000)
  {
    Timers::timers_handler();
    EXPECT(magic_performed == 0);
  }
  // set time to where it should happen
  current_time = 1000000;
  // execute timer interrupt
  Timers::timers_handler();
  // verify timer did execute
  EXPECT(magic_performed == 1);
}

CASE("Start many timers, execute all at once")
{
  current_time = 0;
  magic_performed = 0;
  // start many timers
  for (int i = 0; i < 1000; i++)
    Timers::oneshot(microseconds(i), perform_magic);
  current_time = 1000 * 1000000ull;
  // verify many timers executed
  Timers::timers_handler();
  EXPECT(magic_performed == 1000);
  EXPECT(Timers::active()   == 0);
  EXPECT(Timers::existing() == 1000);
  EXPECT(Timers::free()     == 1000);
  // restore time
  current_time = 0;
}
