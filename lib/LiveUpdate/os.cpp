#include <kernel.hpp>
#include <os.hpp>
#include <kernel/memory.hpp>

#define HIGHMEM_LOCATION  (1ull << 45)
#define LIVEUPDATE_AREA_SIZE 25
static_assert(LIVEUPDATE_AREA_SIZE > 0 && LIVEUPDATE_AREA_SIZE < 50,
              "LIVEUPDATE_AREA_SIZE must be a value between 1 and 50");

//#define LIU_DEBUG 1
#ifdef LIU_DEBUG
#define PRATTLE(fmt, ...) vfprintf(stderr,fmt, ##__VA_ARGS__)
#else
#define PRATTLE(fmt, ...) /* fmt */
#endif

size_t kernel::liveupdate_phys_size(size_t phys_max) noexcept {
  return phys_max / (100 / size_t(LIVEUPDATE_AREA_SIZE));
}
uintptr_t kernel::liveupdate_phys_loc(size_t phys_max)  noexcept {
  return phys_max & ~(uintptr_t) 0xFFF;
}

void kernel::setup_liveupdate(uintptr_t phys)
{
  static uintptr_t temp_phys = 0;
  PRATTLE("Setting up LiveUpdate with phys at %p\n", (void*) phys);
  if (phys != 0) {
    PRATTLE("Deferring setup because too early\n");
    temp_phys = phys; return;
  }
  else {
    PRATTLE("Using temp at %p\n", (void*) temp_phys);
    phys = temp_phys;
  }
  if (kernel::state().liveupdate_loc != 0) return;

  // highmem location
  kernel::state().liveupdate_loc = HIGHMEM_LOCATION;

  // TODO: we really need to find a way to calculate exact area size
  const size_t size = kernel::liveupdate_phys_size(kernel::heap_max());
  if (phys == 0) {
    phys = kernel::liveupdate_phys_loc(kernel::heap_max());
  }
  PRATTLE("New LiveUpdate location: %p\n", (void*) phys);

  // move location to high memory
  const uintptr_t dst = (uintptr_t) kernel::liveupdate_storage_area();
  PRATTLE("virtual_move %p to %p (%zu bytes)\n",
          (void*) phys, (void*) dst, size);
  os::mem::virtual_move(phys, size, dst, "LiveUpdate");
}
