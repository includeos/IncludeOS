#include <os>
#include <kprint>


#include <timer.h>

//#include "../x86_pc/idt.hpp"

extern "C" void noop_eoi() {}
extern "C" void cpu_sampling_irq_handler() {}
extern "C" void blocking_cycle_irq_handler() {}
extern "C" void cpu_disable_all_exceptions();
void (*current_eoi_mechanism)() = noop_eoi;
void (*current_intr_handler)() = nullptr;

void __arch_poweroff()
{

//  kprintf("Disable exceptions\r\n");
  cpu_disable_all_exceptions();
  kprintf("PowerOFF\r\n");
  //TODO check that this is sane on ARM
  //while (1) asm("hlt #0xf000;");
  //exit qemu aarch64
  asm volatile("      \t\n\
    mov w0, 0x18      \t\n\
    mov x1, #0x20000  \t\n\
    add x1, x1, #0x26 \t\n\
    hlt #0xF000       \t\n\
  ");
  __builtin_unreachable();
}

static void platform_init()
{


//  while (timer_get_downcount() > 0);
/*
  int i=0;
  while(1)
  {
    i++;
    if (i%100000)
      printf("Timer count %08x\r\n",timer_get_downcount());

  }

  printf("Timer reached zero\r\n");*/
  // setup CPU exception handlers
  //TODO do this for AARCH64
  //x86::idt_initialize_for_cpu(0);
  //aarch64 exception handlers are already set up ..
  //should we maybe do that from here instead..
}



// not supported!
uint64_t __arch_system_time() noexcept {
  return 0;
}
timespec __arch_wall_clock() noexcept {
  return {0, 0};
}
// not supported!
void OS::block() {}

// default to serial
void OS::default_stdout(const char* str, const size_t len)
{
  __serial_print(str, len);
}

void __arch_reboot(){}
void __arch_enable_legacy_irq(unsigned char){}
void __arch_disable_legacy_irq(unsigned char){}

//doesnt smp belong in aarch and not platform?
void SMP::global_lock() noexcept {}
void SMP::global_unlock() noexcept {}
int SMP::cpu_id() noexcept
{

  return 0;
}
int SMP::cpu_count() noexcept
{
  //cpu_count();
  return 1;
}

void OS::halt() {
  //we died
  printf("OS HALT\r\n");
  cpu_disable_all_exceptions();

  asm volatile("hlt #0xF000");

}

// default stdout/logging states
__attribute__((weak))
bool os_enable_boot_logging = false;
__attribute__((weak))
bool os_default_stdout = false;
