#include "timer.h"
/*
#if defined(__cplusplus)
extern "C" {
#endif
*/

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
void timer_set_downcount(uint32_t count)
{
  //set freq
  asm volatile ("msr cntp_tval_el0, %0" :: "r"(count));
}

uint32_t timer_get_downcount()
{
  uint32_t ret;
  asm volatile ("mrs %0, cntp_tval_el0" : "=r"(ret));
  return ret;
}
void timer_set_control(uint32_t control)
{
    asm volatile("msr cntp_ctl_el0, %0" :: "r"(control));
}
uint32_t timer_get_control()
{
  uint32_t ret;
  asm volatile ("mrs %0, cntp_ctl_el0" : "=r"(ret));
  return ret;
}
void timer_start()
{
  timer_set_control(0x1);
}
/*
#if defined(__cplusplus)
}
#endif
*/
