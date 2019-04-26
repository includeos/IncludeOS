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

void kernel::setup_liveupdate()
{
#if defined(ARCH_x86_64)
  // highmem location
  kernel::state().liveupdate_loc = HIGHMEM_LOCATION;
#else
  // other platforms, without paging
  kernel::state().liveupdate_loc = kernel::state().liveupdate_phys;
#endif

  const size_t size = kernel::state().liveupdate_size;
  PRATTLE("Setting up LiveUpdate from %p to %p, %zx\n",
          (void*) kernel::state().liveupdate_phys,
          (void*) kernel::state().liveupdate_loc, size);

#if defined(ARCH_x86_64)
  os::mem::vmmap().assign_range({
        kernel::state().liveupdate_phys,
        kernel::state().liveupdate_phys + kernel::state().liveupdate_size-1,
        "LiveUpdate physical"
      });

  // move location to high memory
  PRATTLE("virtual_move %p to %p (%zu bytes)\n",
          (void*) kernel::state().liveupdate_phys,
          (void*) kernel::state().liveupdate_loc, size);
  os::mem::virtual_move(kernel::state().liveupdate_phys, size,
                        kernel::state().liveupdate_loc, "LiveUpdate");
#endif
}
