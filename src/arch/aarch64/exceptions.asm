/*
; This file is a part of the IncludeOS unikernel - www.includeos.org
;
; Copyright 2015 Oslo and Akershus University College of Applied Sciences
; and Alfred Bratterud
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.
*/
#include "macros.h"



.globl exception_vector

.macro exception_handler name
.align 7 //start at 80 byte alignement ?
  stp x29,x30, [sp, #-16]!
  bl exception_enter
  bl \name
  b exception_return
.endm

.macro unhandled_exception
  stp x29,x30, [sp, #-16]!
  bl exception_enter
  b exception_unhandled
  b exception_return
.endm

/*
VBAR_ELn
+ 0x000	Synchronous	Current EL with SP0
+ 0x080	IRQ/vIRQ
+ 0x100	FIQ/vFIQ
+ 0x180	SError/vSError
+ 0x200	Synchronous	Current EL with SPx
+ 0x280	IRQ/vIRQ
+ 0x300	FIQ/vFIQ
+ 0x380	SError/vSError
+ 0x400	Synchronous	Lower EL using AArch64
+ 0x480	IRQ/vIRQ
+ 0x500	FIQ/vFIQ
+ 0x580	SError/vSError
+ 0x600	Synchronous	Lower EL using AArch32
+ 0x680	IRQ/vIRQ
+ 0x700	FIQ/vFIQ
+ 0x780	SError/vSError
*/


//2k alignement needed for vector
//TODO register one for each execution level ?
.align 11 //2^11 = 2048
.section .text.exception_table
exception_vector:
  //SP0 Not really tested yet
  exception_handler exception_handler_syn_el
  exception_handler exception_handler_irq_el
  exception_handler exception_handler_fiq_el
  exception_handler exception_handler_serror_el
  //SPX
  exception_handler exception_handler_syn_el
  exception_handler exception_handler_irq_el
  exception_handler exception_handler_fiq_el
  exception_handler exception_handler_serror_el
  //aarch64 el0
  unhandled_exception
  unhandled_exception
  unhandled_exception
  unhandled_exception
  //aarch32 el0
  unhandled_exception
  unhandled_exception
  unhandled_exception
  unhandled_exception

exception_enter:
  //ARM doc suggests a better way of saving regs
  stp  x27, x28, [sp, #-16]!
  stp  x25, x26, [sp, #-16]!
  stp  x23, x24, [sp, #-16]!
  stp  x21, x22, [sp, #-16]!
  stp  x19, x20, [sp, #-16]!
  stp  x17, x18, [sp, #-16]!
  stp  x15, x16, [sp, #-16]!
  stp  x13, x14, [sp, #-16]!
  stp  x11, x12, [sp, #-16]!
  stp  x9, x10, [sp, #-16]!
  stp  x7, x8, [sp, #-16]!
  stp  x5, x6, [sp, #-16]!
  stp  x3, x4, [sp, #-16]!
  stp  x1, x2, [sp, #-16]!
  b save_el

save_el:
  execution_level_switch x12, 3f, 2f, 1f
3:
  mrs x1, esr_el3
  mrs x2, elr_el3
  b 0f
2:
  mrs x1, esr_el2
  mrs x2, elr_el2
  b 0f
1:
  mrs x1, esr_el1
  mrs x2, elr_el1
0:
  stp x2,x0, [sp,#-16]!
  mov x0, sp
  ret

exception_return:
  //restrore EL state
  ldp x2, x0, [sp], #16
  execution_level_switch x12, 3f, 2f, 1f
3:
  msr elr_el3, x2
  b restore_regs
2:
  msr elr_el2, x2
  b restore_regs
1:
  msr elr_el1, x2
  b restore_regs

restore_regs:
  ldp     x1, x2, [sp],#16
  ldp     x3, x4, [sp],#16
  ldp     x5, x6, [sp],#16
  ldp     x7, x8, [sp],#16
  ldp     x9, x10, [sp],#16
  ldp     x11, x12, [sp],#16
  ldp     x13, x14, [sp],#16
  ldp     x15, x16, [sp],#16
  ldp     x17, x18, [sp],#16
  ldp     x19, x20, [sp],#16
  ldp     x21, x22, [sp],#16
  ldp     x23, x24, [sp],#16
  ldp     x25, x26, [sp],#16
  ldp     x27, x28, [sp],#16
  ldp     x29, x30, [sp],#16
  eret
