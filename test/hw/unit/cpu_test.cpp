
#include <common.cxx>

#include <arch.hpp>

CASE("CPU cycle counter")
{
  uint64_t r1 = __arch_cpu_cycles();
  uint64_t r2 = __arch_cpu_cycles();
  EXPECT(r1 != r2);
}
