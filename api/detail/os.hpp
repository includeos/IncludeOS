#include <arch.hpp>
#include <hw/cpu.hpp>

inline uint64_t os::cycles_since_boot() noexcept
{
  return os::Arch::cpu_cycles();
}
inline uint64_t os::nanos_since_boot() noexcept
{
  return cycles_since_boot() / util::GHz(os::cpu_freq()).count();
}
