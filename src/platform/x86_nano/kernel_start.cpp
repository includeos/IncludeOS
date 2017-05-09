#include <kprint>
#include <info>
#include <service>
#include <smp>
#include <boot/multiboot.h>

extern  void default_stdout_handlers();
extern  void __platform_init();
#define MYINFO(X,...) INFO("x86_nano", X, ##__VA_ARGS__)

extern "C" {
  void __init_serial1();
  void __init_sanity_checks();
  void kernel_sanity_checks();
  uintptr_t _multiboot_free_begin(uintptr_t boot_addr);
  uintptr_t _move_symbols(uintptr_t loc);
  void _init_bss();
  void _init_heap(uintptr_t);
  void _init_c_runtime();
  void _init_syscalls();
  void __libc_init_array();
  uintptr_t _end;

  void kernel_start(uintptr_t magic, uintptr_t addr){

    // Initialize serial port 1
    __init_serial1();

    kprintf("\n#include<os> // Literally\n\n");

    //MYINFO("Booting");

    // generate checksums of read-only areas etc.
    __init_sanity_checks();

    // Determine where free memory starts
    uintptr_t free_mem_begin = reinterpret_cast<uintptr_t>(&_end);

    if (magic == MULTIBOOT_BOOTLOADER_MAGIC) {
      free_mem_begin = _multiboot_free_begin(addr);
    }

    // Preserve symbols from the ELF binary
    free_mem_begin += _move_symbols(free_mem_begin);

    // Initialize zero-initialized vars
    _init_bss();

    // Initialize heap
    _init_heap(free_mem_begin);

    //Initialize stack-unwinder, call global constructors etc.
    _init_c_runtime();

    // Initialize system calls
    _init_syscalls();

    //Initialize stdout handlers
    default_stdout_handlers();

    // Call global ctors
    __libc_init_array();

    __platform_init();

    MYINFO("Starting %s\n", Service::name().c_str());

    // Start the service
    Service::start();

    // verify certain read-only sections in memory
    kernel_sanity_checks();

    __arch_poweroff();

  }
}
