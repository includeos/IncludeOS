#include <kernel.hpp>
#include <os.hpp>
#include <kernel/memory.hpp>

#define HIGHMEM_LOCATION  (1ull << 45)
//#define LIU_DEBUG 1
#ifdef LIU_DEBUG
#define PRATTLE(fmt, ...) kprintf(fmt, ##__VA_ARGS__)
#else
#define PRATTLE(fmt, ...) /* fmt */
#endif

void kernel::setup_liveupdate(uintptr_t phys)
{
  static uintptr_t temp_phys;
  PRATTLE("Setting up LiveUpdate with phys at %p\n", (void*) phys);
  if (phys != 0) {
    PRATTLE("Deferring setup because too early\n");
    temp_phys = phys; return;
  }
  else if (phys == 0 && temp_phys == 0) {
    PRATTLE("Setting liveupdate phys to %p\n", (void*) phys);
    phys = kernel::state().liveupdate_phys;
    assert(phys != 0);
  }
  else {
    PRATTLE("Restoring temp at %p\n", (void*) temp_phys);
    phys = temp_phys;
  }
  if (kernel::state().liveupdate_loc != 0) return;

  // highmem location
  kernel::state().liveupdate_loc = HIGHMEM_LOCATION;

  const size_t size = kernel::state().liveupdate_size;
  PRATTLE("New LiveUpdate location: %p, %zx\n", (void*) phys, size);

  os::mem::vmmap().assign_range({
        phys,
        phys + kernel::state().liveupdate_size-1,
        "LiveUpdate physical"
      });

  // move location to high memory
  const uintptr_t dst = (uintptr_t) kernel::liveupdate_storage_area();
  PRATTLE("virtual_move %p to %p (%zu bytes)\n",
          (void*) phys, (void*) dst, size);
  os::mem::virtual_move(phys, size, dst, "LiveUpdate");
}
