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
	%define _stateSize byte
	%define _state 0x7e00	;24 bytes For keeping state info

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
	%include "install_serialHandler.asm"

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


;;; ; Work for eax number of cycles
work:
	xor eax,eax
	mov eax,0x01ffffff
	.work.loop:
	dec eax
	cmp eax,0
	jne .work.loop
	ret

tsc_to_mem:
	rdtsc
	mov dword [_tsc1],eax
	mov dword [_tsc1+4],edx
	ret

tsc_diff:
	rdtsc
	sub eax,[_tsc1]
	sbb edx,[_tsc1+4]
	ret

tsc_qos:
	ret

	%include "PIT_functions.asm"


serialHandler:
;;;  call tsc_to_mem		;Store tsc in memory (_tsc1)
	call work

	call read_al
	inc al
	call print_al

	cli
	mov al,0x20
	out 020h,al
	sti

	iret

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
