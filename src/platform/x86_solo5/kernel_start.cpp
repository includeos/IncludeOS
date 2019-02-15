#include <kprint>
#include <info>
#include <service>
#include <smp>
#include <boot/multiboot.h>
#include <kernel.hpp>
#include <os.hpp>

extern "C" {
#include <solo5/solo5.h>
}

extern void __platform_init();

extern "C" {
  void __init_sanity_checks();
  void kernel_sanity_checks();
  uintptr_t _move_symbols(uintptr_t loc);
  void _init_syscalls();
  void _init_elf_parser();
  uintptr_t _end;
  void set_stack();
  void* get_cpu_ebp();
}

static os::Machine* __machine = nullptr;
os::Machine& os::machine() noexcept {
  Expects(__machine != nullptr);
  return *__machine;
}

static char temp_cmdline[1024];
static uintptr_t mem_size = 0;
static uintptr_t free_mem_begin;

extern "C"
void kernel_start()
{
  // generate checksums of read-only areas etc.
  __init_sanity_checks();

  // Preserve symbols from the ELF binary
  free_mem_begin += _move_symbols(free_mem_begin);

  // Initialize heap
  kernel::init_heap(free_mem_begin, mem_size);

  // Ze machine
  __machine = os::Machine::create((void*)free_mem_begin, mem_size);

  _init_elf_parser();

  // Begin portable HAL initialization
  __machine->init();

  // Initialize system calls
  _init_syscalls();

  // TODO: we should probably actually initialize libc too...
  kernel::state().libc_initialized = true;
  // Initialize OS including devices
  kernel::start(temp_cmdline);
  kernel::post_start();

  // Starting event loop from here allows us to profile OS::start
  os::event_loop();
}

extern "C"
int solo5_app_main(const struct solo5_start_info *si)
{
  // si is stored at 0x6000 by solo5 tender which is used by includeos. Move it fast.
  strncpy(temp_cmdline, si->cmdline, sizeof(temp_cmdline)-1);
  temp_cmdline[sizeof(temp_cmdline)-1] = 0;
  free_mem_begin = si->heap_start;
  mem_size = si->heap_size;

  // set the stack location to its new includeos location, and call kernel_start
  set_stack();
  return 0;
}
