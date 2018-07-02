#include <kprint>
#include <info>
#include <service>
#include <smp>
#include <boot/multiboot.h>
#include <kernel/os.hpp>

extern "C" {
#include <solo5.h>
}

extern void __platform_init();

extern "C" {
  void __init_sanity_checks();
  void kernel_sanity_checks();
  uintptr_t _move_symbols(uintptr_t loc);
  void _init_bss();
  void _init_heap(uintptr_t);
  void _init_syscalls();
  void _init_elf_parser();
  uintptr_t _end;
  void set_stack();
  void* get_cpu_ebp();
}

static char temp_cmdline[1024];
static uintptr_t mem_size = 0;

extern "C"
void kernel_start()
{
  // generate checksums of read-only areas etc.
  __init_sanity_checks();

  // Determine where free memory starts
  uintptr_t free_mem_begin = reinterpret_cast<uintptr_t>(&_end);

  // Preserve symbols from the ELF binary
  free_mem_begin += _move_symbols(free_mem_begin);

  // Do not zero out all solo5 global variables!! == don't touch the BSS
  //_init_bss();

  // Initialize heap
  // XXX: this is dangerous as solo5 might be doing malloc()'s using it's own
  // idea of a heap. Luckily there is no malloc instance at solo5/kernel/[ukvm|virtio|muen],
  // so might be OK (for now).
  OS::init_heap(free_mem_begin, mem_size);

  _init_elf_parser();

  // Initialize system calls
  _init_syscalls();

  // Initialize OS including devices
  OS::start(temp_cmdline);
  OS::post_start();

  // Starting event loop from here allows us to profile OS::start
  OS::event_loop();
}

extern "C"
int solo5_app_main(char* cmdline)
{
   // cmdline is stored at 0x6000 by ukvm which is used by includeos. Move it fast.
   strncpy(temp_cmdline, cmdline, sizeof(temp_cmdline)-1);
   temp_cmdline[sizeof(temp_cmdline)-1] = 0;

   // solo5 sets the stack to be at the end of memory, so let's use that as
   // our memory size (before we change).
   mem_size = (uintptr_t) get_cpu_ebp();

   // set the stack location to its new includeos location, and call kernel_start
   set_stack();
   return 0;
}
