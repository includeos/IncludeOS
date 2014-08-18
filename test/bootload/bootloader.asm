BITS 16
;;; ; Where to store the incrementable output-byte
	%define _CR 0x0D
	%define _LF 0x0A
;;; ; Various ports
	%define _keyboard 60h

	%include "PIT_macros.asm"

;;; ; Memory layout
	%define _boot_segment 0x7c00 ;Turns out this is where you are at boot
	%define _data_segment 0x07c0 ;Data segment, according to mikeOS

		
	_bytesRead dw 0
	_lastByte db 0,0,0,0,"#A" ;Room for 3
	_tsc1 dw 0,0,0,0,"#B"
	_tsc2 dw 0,0,0,0,"##"


	%define _inputBuffer  0x7e20 ; -> Up until stack at 4096


boot:	
	mov ax, _data_segment	; Set data segment to where we're loaded
	mov ds, ax

;;;  Disable the PIT
	mov al,_pit_oneShot	;DISABLE PIT(!) -> only external
	call set_pit_mode
	call set_pit_value	;The mode change won't trigger if no


;;;  SERIAL COMM
;;; Init serial port for output
	mov dx, 0 		;Select COM1
	%include "init_serial_irq.asm"

	;; 	mov al,"!"		
	;; 	call print_al
	mov si, boot_string
	call printstr 

run:
	hlt
	jmp run


;;; ; END OF RUN-LOOP

;;; ; ----------------------
;;; ; ***** FUNCTIONS ******


	%include "PIT_functions.asm"

read_al:			;Read a byte into al
;;;  	call	wait_for_data
	xor edx,edx		;edx must be 0 to read from correct serial port
	mov 	ah, 2		;Opcode for reading from serial
	int 	14h		;Interrupt bios
	ret			;Else, return


printstr:
	lodsb
	cmp al,0
	je .ret
	call print_al
	jmp printstr
	.ret:
	ret

print_al:
	xor edx,edx
	mov ah,1
	int 14h
	ret

boot_string:
	db `IncludeOS Boot loader!\n`

	times 510-($-$$) db 0	;
	dw 0xAA55
