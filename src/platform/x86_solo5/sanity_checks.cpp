
#include <common>
#include <cstdint>

#include <util/crc32.hpp>
#include <kernel/elf.hpp>

#include <kprint>

// Global constructors
static int gconstr_value = 0;
__attribute__((constructor))
static void self_test_gconstr() {
  gconstr_value = 1;
}

extern "C"
void __init_sanity_checks() noexcept
{
}

extern "C"
void kernel_sanity_checks()
{
  // verify that Elf symbols were not overwritten
  bool symbols_verified = Elf::verify_symbols();
  if (!symbols_verified)
    os::panic("Sanity checks: Consistency of Elf symbols and string areas");

  // global constructor self-test
  if (gconstr_value != 1) {
    kprintf("Sanity checks: Global constructors not working (or modified during run-time)!\n");
    os::panic("Sanity checks: Global constructors verification failed");
  }
}
