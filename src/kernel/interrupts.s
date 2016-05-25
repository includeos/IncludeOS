// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//Macros require compilation with the '-x assembler-with-cpp' option
#include "pic_defs.h"
#define PIC_PORT $0x20
#define SIG_EOI $0x20

.global irq_default_entry
.global irq_timer_entry
    ;; .global irq_virtio_entry

.global exception_entry
//21-29 are reserved

//Redirected IRQ 0 - 15
.global irq_0_entry
.global irq_1_entry
//The 3rd irq is the slave PIC one so it doesn't need an entry
.global irq_3_entry
.global irq_4_entry
.global irq_5_entry
.global irq_6_entry
.global irq_7_entry
.global irq_8_entry
.global irq_9_entry
.global irq_10_entry
.global irq_11_entry
.global irq_12_entry
.global irq_13_entry
.global irq_14_entry
.global irq_15_entry

.global cpu_sampling_irq_entry
.global ide_irq_entry
.extern ide_irq_handler


/*
	IRQ entry wrapper
	Default behavior for IRQ- and exception handling
*/
.macro IRQ name call
\name:
	cli
	pusha

	call \call
// Send EOI - we're doing that in the handlers
	sti

	popa
	iret
.endm

IRQ exception_entry exception_handler
//   exception 21 - 29 are reserved

IRQ irq_0_entry irq_0_handler
IRQ irq_1_entry irq_1_handler
IRQ irq_3_entry irq_3_handler
IRQ irq_4_entry irq_4_handler
IRQ irq_5_entry irq_5_handler
IRQ irq_6_entry irq_6_handler
IRQ irq_7_entry irq_7_handler
IRQ irq_8_entry irq_8_handler
IRQ irq_9_entry irq_9_handler
IRQ irq_10_entry irq_10_handler
IRQ irq_11_entry irq_11_handler
IRQ irq_12_entry irq_12_handler
IRQ irq_13_entry irq_13_handler
IRQ irq_14_entry irq_14_handler
IRQ irq_15_entry irq_15_handler

IRQ ide_irq_entry ide_irq_handler
IRQ cpu_sampling_irq_entry cpu_sampling_irq_handler
IRQ irq_default_entry irq_default_handler

exception_13_entry:
	cli
	call exception_13_handler

.global modern_interrupt_handler
modern_interrupt_handler:
  pusha
  call register_modern_interrupt
  popa
  iret

irq_timer_entry:
	cli

	pusha
	//push $'d'
	call irq_timer_handler

	//Send EOI for the timer
	movb	$PIC1, %al
	movw	$PIC1, %dx
	outb	%al, %dx


	popa
	sti

	iret
