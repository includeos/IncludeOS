	%define pic1 0x20
	%define pic2 0xA0
	;; 	xor ax, 04h 	;Disable flag 4 - activates IRQ 4
	;; 	mov _outbyte, "s"
	xor ax,ax
	
	;; 	%include "pic_init_orig.asm"
	
 	in al,pic1+1		;Get PIC-flags
	and al,0xef		;- enable the 4th-bit also	
	out pic1+1,al		;SET PIC-flags		

	xor ax,ax		;Clear ax

	;; SET the serial chip 
	mov dx,3f9h		;0x3F8 is base address for COM1,Put it in x
	in al,dx		;+1 is IER (Interrupt Enable Register)

	cli
	mov al,1		;Enable serial interrupt flag (set bit 0 high)	
	out dx,al		;Write it back
	sti

	;; Reset dx, which was clobbered
	xor dx,dx
	;; 	out 21h, ax		;Set PIC-flags
	;	call print1_serial	;
