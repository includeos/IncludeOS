//Macros require compilation with the '-x assembler-with-cpp' option
#include "irq/pic_defs.h"
#define PIC_PORT $0x20
#define SIG_EOI $0x20

.global default_irq_entry
.global timer_irq_entry
.global irq_0_entry
.global irq_1_entry	
.global irq_2_entry
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

/*
	IRQ entry Skeleton
	Default behavior for IRQ handling
*/	
.macro irq name call
\name:
	cli
	pusha

	//Send EOI for the timer
	movb	$PIC1, %al
	movw	$PIC1, %dx
	outb	%al, %dx
	sti
	
	call \call
	popa
	iret
.endm

IRQ irq_0_entry irq_0_handler
IRQ irq_1_entry irq_1_handler
IRQ irq_2_entry irq_2_handler
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
	
	
default_irq_entry:	
	cli
	
	pusha

	sub $8,%esp
	mov %cr0,%eax
	mov %eax, 0(%esp)
	movl $0xbeef, 4(%esp)
	movl $0xb055, 8(%esp)
	
	//push $'d'
	call default_irq_handler

	//Send EOI for the timer
	movb	PIC_PORT, %al
	movw	SIG_EOI, %dx
	outb	%al, %dx

	add $8,%esp
	
	popa
	sti
	
	iret

timer_irq_entry:	
	cli
	
	pusha	
	//push $'d'
	call timer_irq_handler

	//Send EOI for the timer
	movb	$PIC1, %al
	movw	$PIC1, %dx
	outb	%al, %dx

	
	popa
	sti
	
	iret
