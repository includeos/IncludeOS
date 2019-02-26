#include <kernel/os.hpp>
#include <kernel/memory.hpp>

#define HIGHMEM_LOCATION  (1ull << 45)
#define LIVEUPDATE_AREA_SIZE 25
static_assert(LIVEUPDATE_AREA_SIZE > 0 && LIVEUPDATE_AREA_SIZE < 50,
              "LIVEUPDATE_AREA_SIZE must be a value between 1 and 50");

static uintptr_t temp_phys = 0;

//#define LIU_DEBUG 1
#ifdef LIU_DEBUG
#define PRATTLE(fmt, ...) vfprintf(stderr,fmt, ##__VA_ARGS__)
#else
#define PRATTLE(fmt, ...) /* fmt */
#endif

size_t OS::liveupdate_phys_size(size_t phys_max) noexcept {
  return phys_max / (100 / size_t(LIVEUPDATE_AREA_SIZE));
};

uintptr_t OS::liveupdate_phys_loc(size_t phys_max)  noexcept {
  return (phys_max - liveupdate_phys_size(phys_max)) & ~(uintptr_t) 0xFFF;
};

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
    size = OS::liveupdate_phys_size(OS::heap_max());
    phys = OS::liveupdate_phys_loc(OS::heap_max());
  }
  else {
    size = OS::heap_max() - phys;
  }
  size &= ~(uintptr_t) 0xFFF; // page sized

  // move location to high memory
  const uintptr_t dst = (uintptr_t) OS::liveupdate_storage_area();
  PRATTLE("virtual_move %p to %p (%zu bytes)\n",
          (void*) phys, (void*) dst, size);
  os::mem::virtual_move(phys, size, dst, "LiveUpdate");
}
