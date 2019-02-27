#include "timer.h"

void timer_set_frequency(uint32_t freq)
{
  //set freq
  asm volatile ("msr cntfrq_el0, %0" :: "r"(freq));
}

uint32_t timer_get_frequency()
{
  uint32_t ret;
  asm volatile ("mrs %0, cntfrq_el0" : "=r"(ret));
  return ret;
}

//virtual reg is called cntv_tval_el0 use el to check ?
void timer_set_count(uint32_t count)
{
  //set freq
  asm volatile ("msr cntp_tval_el0, %0" :: "r"(count));
}

uint64_t timer_get_virtual_countval()
{
  uint64_t cntvct_el0;
  asm volatile("mrs %0, cntvct_el0" : "=r" (cntvct_el0) : : "memory");
  return cntvct_el0;
}

uint64_t timer_get_virtual_compare()
{
  uint64_t cntvct_el0;
  asm volatile("mrs %0, cntv_cval_el0" : "=r" (cntvct_el0) : : "memory");
  return cntvct_el0;
}

void timer_set_virtual_compare(uint64_t compare)
{
  asm volatile("msr cntv_cval_el0 , %0" :: "r" (compare) : "memory");
}

void timer_set_virtual_control(uint32_t val)
{
  asm volatile("msr cntv_ctl_el0 , %0" :: "r" (val) : "memory");
}

uint32_t timer_get_virtual_control()
{
  uint32_t ctl;
  asm volatile("mrs %0, cntv_ctl_el0" : "=r" (ctl) : : "memory");
  return ctl;
}

void timer_virtual_stop()
{
  timer_set_virtual_control(0x0);
}

void timer_virtual_start()
{
  timer_set_virtual_control(0x1);
  //timer
}


uint32_t timer_get_count()
{
  uint32_t ret;
  asm volatile ("mrs %0, cntp_tval_el0" : "=r"(ret));
  return ret;
}
void timer_set_control(uint32_t control)
{
    asm volatile("msr cntp_ctl_el0, %0" :: "r"(control): "memory");
}
uint32_t timer_get_control()
{
  uint32_t ret;
  asm volatile ("mrs %0, cntp_ctl_el0" : "=r"(ret) :: "memory");
  return ret;
}
void timer_stop()
{
//  uint32_t ctl=timer_get_control();
//  ctl &=~(0x1);
  timer_set_control(0x0);
}
void timer_start()
{
  uint32_t ctl=timer_get_control();
  ctl |=(0x1);
  timer_set_control(ctl);
}
