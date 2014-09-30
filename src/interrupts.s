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

//Redirected IRQ 0 - 12
.global irq_32_entry
.global irq_33_entry
.global irq_34_entry
.global irq_35_entry
.global irq_36_entry
.global irq_37_entry    
.global irq_38_entry
.global irq_39_entry
.global irq_40_entry
.global irq_41_entry
.global irq_42_entry
.global irq_43_entry    
     
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

IRQ exception_0_entry exception_0_handler
IRQ exception_1_entry exception_1_handler
IRQ exception_2_entry exception_2_handler
IRQ exception_3_entry exception_3_handler
IRQ exception_4_entry exception_4_handler
IRQ exception_5_entry exception_5_handler
IRQ exception_6_entry exception_6_handler
IRQ exception_7_entry exception_7_handler		
IRQ exception_8_entry exception_8_handler	
IRQ exception_9_entry exception_9_handler
IRQ exception_10_entry exception_10_handler	
IRQ exception_11_entry exception_11_handler
IRQ exception_12_entry exception_12_handler
IRQ exception_13_entry exception_13_handler
IRQ exception_14_entry exception_14_handler
IRQ exception_15_entry exception_15_handler
IRQ exception_16_entry exception_16_handler
IRQ exception_17_entry exception_17_handler
IRQ exception_18_entry exception_18_handler
IRQ exception_19_entry exception_19_handler
IRQ exception_20_entry exception_20_handler
//   exception 21 - 29 are reserved	
IRQ exception_30_entry exception_30_handler
IRQ exception_31_entry exception_31_handler

IRQ irq_32_entry irq_32_handler
IRQ irq_33_entry irq_33_handler
IRQ irq_34_entry irq_34_handler
IRQ irq_35_entry irq_35_handler
IRQ irq_36_entry irq_36_handler
IRQ irq_37_entry irq_37_handler
    
IRQ irq_38_entry irq_38_handler
IRQ irq_39_entry irq_39_handler
IRQ irq_40_entry irq_40_handler
IRQ irq_41_entry irq_41_handler
IRQ irq_42_entry irq_42_handler
IRQ irq_43_entry irq_43_handler



    
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

