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

void timer_set_count(uint32_t count);
uint32_t timer_get_count();
uint64_t timer_get_virtual_countval();
uint64_t timer_get_virtual_compare();
uint32_t timer_get_control();
void timer_set_control();
void timer_set_virtual_compare(uint64_t compare);

void timer_set_virtual_control(uint32_t val);

uint32_t timer_get_virtual_control();

void timer_virtual_stop();
void timer_virtual_start();
//void timer_start_downcount();
void timer_start();
void timer_stop();

#if defined(__cplusplus)
}
#endif


#endif //AARCH64TIMER
