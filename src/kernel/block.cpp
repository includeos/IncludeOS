
#include <os>
#include <statman>
#include <kernel/events.hpp>

// Keep track of blocking levels
static uint32_t* blocking_level = nullptr;
static uint32_t* highest_blocking_level = nullptr;

// Getters, mostly for testing
extern "C" uint32_t os_get_blocking_level() {
  return *blocking_level;
}
extern "C" uint32_t os_get_highest_blocking_level() {
  return *highest_blocking_level;
}

/**
 * A quick and dirty implementation of blocking calls, which simply halts,
 * then calls  the event loop, then returns.
 **/
void os::block() noexcept
{
  // Initialize stats
  if (not blocking_level) {
    blocking_level = &Statman::get()
      .create(Stat::UINT32, std::string("blocking.current_level")).get_uint32();
    *blocking_level = 0;
  }

  if (not highest_blocking_level) {
    highest_blocking_level = &Statman::get()
      .create(Stat::UINT32, std::string("blocking.highest")).get_uint32();
    *highest_blocking_level = 0;
  }

  // Increment level
  *blocking_level += 1;

  // Increment highest if applicable
  if (*blocking_level > *highest_blocking_level)
    *highest_blocking_level = *blocking_level;

  // Process immediate events
  Events::get().process_events();

  // Await next interrupt
  os::halt();

  // Process events (again?)
  Events::get().process_events();

  // Decrement level
  *blocking_level -= 1;
}
