
#include <common.cxx>
#include <kernel/rng.hpp>

CASE("RNG init")
{
  uint32_t value;
  rng_absorb(&value, 4);
}
CASE("RNG rng_extract")
{
  uint32_t value = 0;
  while (value == 0) {
    value = rng_extract_uint32();
  }
  EXPECT(value != 0);
  // chance should be pretty low
  uint32_t value2 = rng_extract_uint32();
  EXPECT(value2 != value);
}
