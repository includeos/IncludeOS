
#include <hal/machine.hpp>
#include <util/units.hpp>
#include <kernel.hpp>
#include <os>

//#define INFO_MACHINE
#ifdef INFO_MACHINE
#ifndef USERSPACE_KERNEL
#define MINFO(fmt, ...) kprintf("[ Machine ] " fmt, ##__VA_ARGS__)
#else
#define MINFO(fmt, ...) printf("[ Machine ] " fmt, ##__VA_ARGS__)
#endif
#else
#define MINFO(...) /* */
#endif

using namespace util;

// Reserve some machine memory for e.g. devices
// (can still be used by heap as fallback).
static constexpr size_t reserve_mem = 1_KiB;

// Max percent of memory reserved by machine
static constexpr int  reserve_pct_max = 10;
static_assert(reserve_pct_max > 0 and reserve_pct_max < 90);

namespace os {
    __attribute__((weak))
    uintptr_t liveupdate_memory_size_mb = 0;

  Machine::Memory& Machine::memory() noexcept {
    return impl->memory();
  }

  void Machine::init() noexcept {
    impl->init();
  }

  const char* Machine::arch() noexcept {
    return impl->arch();
  }

  // Implementation details
  Machine* Machine::create(void* mem, size_t size) noexcept {
    char* mem_begin = (char*)mem + sizeof(Machine);
    return new(mem) Machine((void*)mem_begin, size - sizeof(Machine));
  }

  Machine::Machine(void* mem, size_t size) noexcept
  : impl {nullptr} {

    Expects(mem != nullptr);
    Expects(size > sizeof(detail::Machine) + Machine::Memory::min_alloc);

    // Placement new impl
    impl = {new (mem) detail::Machine{(char*)mem + sizeof(detail::Machine),
                                      size - sizeof(detail::Machine)}};
  }

}

// Detail implementations
namespace os::detail {

  Machine::Machine(void* raw_mem, size_t size)
    : mem_{
        (void*) bits::align(Memory::align, (uintptr_t)raw_mem),
        size - (bits::align(Memory::align, (uintptr_t)raw_mem) - (uintptr_t)raw_mem)
      },
      ptr_alloc_(mem_), parts_(ptr_alloc_), device_types_(mem_) {
          
      }

  void Machine::init() {
    MINFO("Initializing heap\n");
    auto main_mem = memory().allocate_largest();
    MINFO("Main memory detected as %zu b\n", main_mem.size);
    memory().deallocate(main_mem.ptr, main_mem.size);

#ifndef PLATFORM_x86_solo5
    static const size_t SYSTEMLOG_SIZE = 65536;
    const size_t LIU_SIZE = os::liveupdate_memory_size_mb * 1024 * 1024;
    auto liu_mem = memory().allocate_back(LIU_SIZE + SYSTEMLOG_SIZE);
    kernel::state().liveupdate_phys = (uintptr_t) liu_mem.ptr + SYSTEMLOG_SIZE;
    kernel::state().liveupdate_size = liu_mem.size - SYSTEMLOG_SIZE;
#endif

    // reallocate main memory from remainder
    main_mem = memory().allocate_largest();

    MINFO("Reserving %zu b for machine use\n", reserve_mem);
    main_mem.size -= reserve_mem;
    auto back = (uintptr_t)main_mem.ptr + main_mem.size - reserve_mem;
    MINFO("Deallocating %zu b from back of machine\n", reserve_mem);
    memory().deallocate((void*)back, reserve_mem);

    kernel::init_heap((uintptr_t) main_mem.ptr, (uintptr_t) main_mem.ptr + main_mem.size);
  }

  const char* Machine::arch() { return Arch::name; }
}
