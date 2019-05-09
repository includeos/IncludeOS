#include <common.cxx>
#include <os.hpp>
#include <kernel/events.hpp>
extern "C" uint32_t os_get_blocking_level();
extern "C" uint32_t os_get_highest_blocking_level();

CASE("OS::block")
{
  static bool event_happened = false;
  auto ev = Events::get().subscribe(
    [&] () {
      event_happened = true;
      EXPECT(os_get_blocking_level() == 1);
      EXPECT(os_get_highest_blocking_level() == 1);
    });
  Events::get().trigger_event(ev);
  // block until event happens
  os::block();
  // Done
  EXPECT(event_happened);
  EXPECT(os_get_blocking_level() == 0);
  EXPECT(os_get_highest_blocking_level() == 1);
}
