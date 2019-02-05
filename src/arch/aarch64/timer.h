#pragma once
#ifndef AARCH64_TIMER
#define AARCH64TIMER
//64 bit
//should it be EL0 or EL1 ?
//READTHEDOCS!!
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

void timer_set_frequency(uint32_t freq);
uint32_t timer_get_frequency();

void timer_set_downcount(uint32_t count);
uint32_t timer_get_downcount();

void timer_start_downcount();
void timer_start();

#if defined(__cplusplus)
}
#endif


#endif //AARCH64TIMER
