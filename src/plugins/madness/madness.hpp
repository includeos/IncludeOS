
#ifndef PLUGINS_MADNESS_HPP
#define PLUGINS_MADNESS_HPP

// Cause trouble for services. Useful for robustness testing.

namespace madness {
  using namespace std::chrono;
  constexpr auto alloc_freq = 1s;
  constexpr auto dealloc_delay = 5s;
  constexpr auto alloc_restart_delay = 60s;
  constexpr size_t alloc_min  = 0x1000;

  /* Periodically allocate all possible memory, before releasing */
  void init_heap_steal();
  void init_status_printing();
  void init();

} // namespace madness
#endif
