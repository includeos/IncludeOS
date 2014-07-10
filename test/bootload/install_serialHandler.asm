	SERIAL_INTERRUPT EQU 0x0C
	;; INSTALL Serial interrupt
	push ds
	push word 0
	pop ds
	cli
	mov [4 * SERIAL_INTERRUPT], word _boot_segment+serialHandler
	mov [4 * SERIAL_INTERRUPT + 2],cs
	sti
	pop ds
