#pragma once
#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#define EL0               0
#define EL1               1
#define EL2               2
#define EL3               3

/*
//exception flags ?
enum class cpu_irq_flag_t: uint32_t
{
//  IRQ_FLAG_T = 1 <<5 //reserved (RESERVED) aarch64 used for exception from aarch32//
  IRQ_FLAG_F = 1<<6, //FIQ
  IRQ_FLAG_I = 1<<7, //IRQ
  IRQ_FLAG_A = 1<<8, //SError
  IRQ_FLAG_D = 1<<9 //Endianess in AARCH32 and Debug exception mask in aarch64
};// cpu_irq_flag_t;
*/
#if defined(__cplusplus)
extern "C" {
#endif


void cpu_fiq_enable();

void cpu_irq_enable();

void cpu_serror_enable();

void cpu_debug_enable();

void cpu_fiq_disable();

void cpu_irq_disable();

void cpu_serror_disable();

void cpu_debug_disable();

void cpu_disable_all_exceptions();

void cpu_wfi();

void cpu_disable_exceptions(uint32_t irq);
void cpu_enable_exceptions(uint32_t irq);

uint32_t cpu_get_current_el();
void cpu_print_current_el();

#if defined(__cplusplus)
}
#endif


#endif //CPU_H
