#include <kernel/os.hpp>
#include <kernel/memory.hpp>
#include <kprint>

#define HIGHMEM_LOCATION  (1ull << 45)
static uintptr_t temp_phys = 0;

//#define LIU_DEBUG 1
#ifdef LIU_DEBUG
#define PRATTLE(fmt, ...) kprintf(fmt, ##__VA_ARGS__)
#else
#define PRATTLE(fmt, ...) /* fmt */
#endif

void OS::setup_liveupdate(uintptr_t phys)
{
  PRATTLE("Setting up LiveUpdate with phys at %p\n", (void*) phys);
  if (phys != 0) {
    PRATTLE("Deferring setup because too early\n");
    temp_phys = phys; return;
  }
  if (OS::liveupdate_loc_ != 0) return;
  PRATTLE("New virtual move heap_max: %p\n", (void*) OS::heap_max());

  // highmem location
  OS::liveupdate_loc_ = HIGHMEM_LOCATION;

  size_t size = 0;
  if (phys == 0) {
    // default is 1/4 of heap from the end of memory
    size = OS::heap_max() / 4;
    phys = (OS::heap_max() - size) & 0xFFFFFFF0;
  }
  else {
    size = OS::heap_max() - phys;
  }

  // move location to high memory
  const uintptr_t dst = (uintptr_t) OS::liveupdate_storage_area();
  PRATTLE("virtual_move %p to %p (%zu bytes)\n",
          (void*) phys, (void*) dst, size);
  os::mem::virtual_move(phys, size, dst, "LiveUpdate");
}
