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
.global irq_32_entry
.global irq_33_entry
//The 34th irq is the slave PIC one so it doesn't need an entry
.global irq_35_entry
.global irq_36_entry
.global irq_37_entry
.global irq_38_entry
.global irq_39_entry
.global irq_40_entry
.global irq_41_entry
.global irq_42_entry
.global irq_43_entry
.global irq_44_entry
.global irq_45_entry
.global irq_46_entry
.global irq_47_entry

.global cpu_sampling_irq_entry
	
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

IRQ irq_32_entry irq_32_handler
IRQ irq_33_entry irq_33_handler
IRQ irq_35_entry irq_35_handler
IRQ irq_36_entry irq_36_handler
IRQ irq_37_entry irq_37_handler
    
IRQ irq_38_entry irq_38_handler
IRQ irq_39_entry irq_39_handler
IRQ irq_40_entry irq_40_handler
IRQ irq_41_entry irq_41_handler
IRQ irq_42_entry irq_42_handler
IRQ irq_43_entry irq_43_handler
IRQ irq_44_entry irq_44_handler
IRQ irq_45_entry irq_45_handler
IRQ irq_46_entry irq_46_handler
IRQ irq_47_entry irq_47_handler


exception_13_entry:
	cli
	call exception_13_handler


cpu_sampling_irq_entry:
	cli
	pusha
	call cpu_sampling_irq_handler
	popa
	sti
	iret
	
	
irq_default_entry:	
	cli
	pusha
	call irq_default_handler
	popa
	sti	
	iret


//Send EOI for the timer
//      movb	PIC_PORT, %al
//	movw	SIG_EOI, %dx
//	outb	%al, %dx


    
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

