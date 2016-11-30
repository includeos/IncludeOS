#include <os>
#include <kprint>
#include <hw/apic_timer.hpp>

namespace hw {
  uint32_t apic_timer_get_ticks();
  void     apic_timer_set_ticks(uint32_t);
}

struct softreset_t
{
  MHz      cpu_freq;
  uint32_t apic_ticks;
};

void OS::resume_softreset(uint32_t addr)
{
  kprint("Soft resetting OS\n");
  auto* data = (softreset_t*) addr;
  
  OS::cpu_mhz_ = data->cpu_freq;
  hw::apic_timer_set_ticks(data->apic_ticks);
}

extern "C"
void* __os_store_soft_reset()
{
  // store softreset data in low memory
  auto* ptr = (softreset_t*) 0x7000;
  ptr->cpu_freq   = OS::cpu_freq();
  ptr->apic_ticks = hw::apic_timer_get_ticks();
  return ptr;
}
