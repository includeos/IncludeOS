#include <kprint>
#include <info>
#include <service>
#include <smp>
#include <boot/multiboot.h>
#include <kernel/os.hpp>

extern "C" {
#include <solo5.h>
}

extern  void solo5_stdout_handlers();
extern  void __platform_init();

extern char _MULTIBOOT_START_;

/* os */
#include <statman>
#include <kernel/rng.hpp>

extern "C" void* get_cpu_esp();
extern "C" void  kernel_sanity_checks();
extern uintptr_t heap_begin;
extern uintptr_t heap_end;
extern uintptr_t _start;
extern uintptr_t _end;
extern uintptr_t _ELF_START_;
extern uintptr_t _TEXT_START_;
extern uintptr_t _LOAD_START_;
extern uintptr_t _ELF_END_;

#define MYINFO(X,...) INFO("Kernel", X, ##__VA_ARGS__)

#ifdef ENABLE_PROFILERS
#include <profile>
#define PROFILE(name)  ScopedProfiler __CONCAT(sp, __COUNTER__){name};
#else
#define PROFILE(name) /* name */
#endif
/* os */

void OS::add_stdout_solo5()
{
  add_stdout(
  [] (const char* str, const size_t len) {
    solo5_console_write(str, len);
  });
}

__attribute__ ((weak))
void solo5_stdout_handlers()
{
  OS::add_stdout_solo5();
}


void OS::start_solo5(uint32_t boot_magic, uint32_t boot_addr)
{
  PROFILE("");
  // Print a fancy header
  CAPTION("#include<os> // Literally");

  void* esp = get_cpu_esp();
  MYINFO("Stack: %p", esp);
  MYINFO("Boot magic: 0x%x, addr: 0x%x",
         boot_magic, boot_addr);

  /// STATMAN ///
  /// initialize on page 7, 2 pages in size
  Statman::get().init(0x6000, 0x3000);

  printf("multiboot addr %u\n", boot_addr);

  PROFILE("Multiboot / legacy");
  // Detect memory limits etc. depending on boot type
  if (boot_magic == MULTIBOOT_BOOTLOADER_MAGIC) {
    OS::multiboot(boot_addr);
  } else {

    if (is_softreset_magic(boot_magic) && boot_addr != 0)
        OS::resume_softreset(boot_addr);

    OS::legacy_boot();
  }

  Expects(high_memory_size_);

  PROFILE("Memory map");
  // Assign memory ranges used by the kernel
  auto& memmap = memory_map();

  OS::memory_end_ = high_memory_size_ + 0x100000;
  MYINFO("Assigning fixed memory ranges (Memory map)");

  memmap.assign_range({0x6000, 0x8fff, "Statman", "Statistics"});
  memmap.assign_range({0xA000, 0x9fbff, "Stack", "Kernel / service main stack"});
  memmap.assign_range({(uintptr_t)&_LOAD_START_, (uintptr_t)&_end,
        "ELF", "Your service binary including OS"});

  Expects(::heap_begin and heap_max_);
  // @note for security we don't want to expose this
  memmap.assign_range({(uintptr_t)&_end + 1, ::heap_begin - 1,
        "Pre-heap", "Heap randomization area"});

  // Give the rest of physical memory to heap
  heap_max_ = ((0x100000 + high_memory_size_)  & 0xffff0000) - 1;

  uintptr_t span_max = std::numeric_limits<std::ptrdiff_t>::max();
  uintptr_t heap_range_max_ = std::min(span_max, heap_max_);

  MYINFO("Assigning heap");
  memmap.assign_range({::heap_begin, heap_range_max_,
        "Heap", "Dynamic memory", heap_usage });

  MYINFO("Printing memory map");

  for (const auto &i : memmap)
    INFO2("* %s",i.second.to_string().c_str());

  // sleep statistics
  // XXX need these back
  // NOTE: needs to be positioned before anything that calls OS::halt
 // os_cycles_hlt = &Statman::get().create(
   //   Stat::UINT64, std::string("cpu0.cycles_hlt")).get_uint64();
  //os_cycles_total = &Statman::get().create(
    //  Stat::UINT64, std::string("cpu0.cycles_total")).get_uint64();

  PROFILE("Platform init");
  extern void __platform_init();
  __platform_init();

  //PROFILE("RTC init");
  // Realtime/monotonic clock
  //RTC::init();

  MYINFO("Initializing RNG");
  PROFILE("RNG init");
  RNG::init();

  // Seed rand with 32 bits from RNG
  srand(rng_extract_uint32());

  // Custom initialization functions
  MYINFO("Initializing plugins");
  // the boot sequence is over when we get to plugins/Service::start
  OS::boot_sequence_passed_ = true;

  PROFILE("Plugins init");
  for (auto plugin : plugins_) {
    INFO2("* Initializing %s", plugin.name_);
    try{
      plugin.func_();
    } catch(std::exception& e){
      MYINFO("Exception thrown when initializing plugin: %s", e.what());
    } catch(...){
      MYINFO("Unknown exception when initializing plugin");
    }
  }

  PROFILE("Service::start");
  // begin service start
  FILLINE('=');
  printf(" IncludeOS %s (%s / %i-bit)\n",
         version().c_str(), arch().c_str(),
         static_cast<int>(sizeof(uintptr_t)) * 8);
  printf(" +--> Running [ %s ]\n", Service::name().c_str());
  FILLINE('~');

  Service::start();
  // NOTE: this is a feature for service writers, don't move!

  kernel_sanity_checks();
}


extern "C" {
  void __init_sanity_checks();
  void kernel_sanity_checks();
  uintptr_t _multiboot_free_begin(uintptr_t boot_addr);
  uintptr_t _move_symbols(uintptr_t loc);
  void _init_bss();
  void _init_heap(uintptr_t);
  void _init_c_runtime();
  void _init_syscalls();
  void __libc_init_array();

  void solo5_poweroff()
  {
    __asm__ __volatile__("cli; hlt");
    for(;;);
  }

  void kernel_start(char *cmdline){

    // generate checksums of read-only areas etc.
    // XXX this takes 40ms
    __init_sanity_checks();

    // Determine where free memory starts
    uintptr_t free_mem_begin = reinterpret_cast<uintptr_t>(&_end);

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
    solo5_stdout_handlers();

    // Call global ctors
    __libc_init_array();

    // Initialize OS including devices

    /* Horrible hack to set __multiboot_addr, which is 32 bits, from a
     * pointer to &_MULTIBOOT_START_, which is 64 bits.
     */
    extern uint32_t __multiboot_addr;
    uint64_t mb_64 = (uint64_t) &_MULTIBOOT_START_;
    __multiboot_addr = 1 + mb_64 - 1;
    multiboot_info_t *bootinfo =  (multiboot_info_t*) &_MULTIBOOT_START_;
    //bootinfo->flags = MULTIBOOT_INFO_MEMORY | MULTIBOOT_INFO_CMDLINE;
    bootinfo->flags = MULTIBOOT_INFO_MEMORY;
    bootinfo->mem_lower = 639; // copied from a regular includeos run
    bootinfo->mem_upper = 524288 - (0xeffff / 1024); // ukvm mem_size to kb
    //std::strcpy((char *) bootinfo->cmdline, cmdline);

    OS::start_solo5(MULTIBOOT_BOOTLOADER_MAGIC, __multiboot_addr);

    // Starting event loop from here allows us to profile OS::start
    OS::event_loop();

    solo5_poweroff();
  }

  int solo5_app_main(char *cmdline) {
     kernel_start(cmdline);
  }
}
