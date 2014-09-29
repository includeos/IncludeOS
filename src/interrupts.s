//Macros require compilation with the '-x assembler-with-cpp' option
#include "irq/pic_defs.h"
#define PIC_PORT $0x20
#define SIG_EOI $0x20

.global irq_default_entry
.global irq_timer_entry
    ;; .global irq_virtio_entry
    
.global exception_0_entry
.global exception_1_entry	
.global exception_2_entry
.global exception_3_entry
.global exception_4_entry
.global exception_5_entry	
.global exception_6_entry
.global exception_7_entry
.global exception_8_entry
.global exception_9_entry
.global exception_10_entry
.global exception_11_entry
.global exception_12_entry
.global exception_13_entry
.global exception_14_entry
.global exception_15_entry
.global exception_16_entry
.global exception_17_entry
.global exception_18_entry
.global exception_19_entry
.global exception_20_entry
//21-29 are reserved

.global exception_30_entry
.global exception_31_entry	


/*
	IRQ entry Skeleton
	Default behavior for IRQ handling
*/	
.macro EXPT name call
\name:
	cli
	pusha

	//Send EOI for the timer
//	movb	$PIC1, %al
//	movw	$PIC1, %dx
//	outb	%al, %dx


	call \call
	sti

	popa
	iret
.endm

EXPT exception_0_entry exception_0_handler
EXPT exception_1_entry exception_1_handler
EXPT exception_2_entry exception_2_handler
EXPT exception_3_entry exception_3_handler
EXPT exception_4_entry exception_4_handler
EXPT exception_5_entry exception_5_handler
EXPT exception_6_entry exception_6_handler
EXPT exception_7_entry exception_7_handler		
EXPT exception_8_entry exception_8_handler	
EXPT exception_9_entry exception_9_handler
EXPT exception_10_entry exception_10_handler	
EXPT exception_11_entry exception_11_handler
EXPT exception_12_entry exception_12_handler
EXPT exception_13_entry exception_13_handler
EXPT exception_14_entry exception_14_handler
EXPT exception_15_entry exception_15_handler
EXPT exception_16_entry exception_16_handler
EXPT exception_17_entry exception_17_handler
EXPT exception_18_entry exception_18_handler
EXPT exception_19_entry exception_19_handler
EXPT exception_20_entry exception_20_handler
//   exception 21 - 29 are reserved	
EXPT exception_30_entry exception_30_handler
EXPT exception_31_entry exception_31_handler	


	
irq_default_entry:	
	cli
	
	pusha
	
	//push $'d'
	call irq_default_handler

	//Send EOI for the timer
	movb	PIC_PORT, %al
	movw	SIG_EOI, %dx
	outb	%al, %dx
	
	popa
	sti
	
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

