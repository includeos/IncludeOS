
#include <kernel/elf.hpp>
#include <kprint>
#include <os.hpp>
//#define TRUST_BUT_VERIFY

// Global constructors
static int gconstr_value = 0;
__attribute__((constructor, used))
static void self_test_gconstr() {
  gconstr_value = 1;
}

extern "C"
void kernel_sanity_checks()
{
#ifdef TRUST_BUT_VERIFY
  // verify that Elf symbols were not overwritten
  bool symbols_verified = Elf::verify_symbols();
  if (!symbols_verified)
    os::panic("Sanity checks: Consistency of Elf symbols and string areas");
#endif

  // global constructor self-test
  if (gconstr_value != 1) {
    kprintf("Sanity checks: Global constructors not working (or modified during run-time)!\n");
    os::panic("Sanity checks: Global constructors verification failed");
  }
}
