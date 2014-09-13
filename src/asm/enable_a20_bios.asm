enable_a20_bios:	
	;; Enable A20 using the BIOS
	mov ax, 0x2401	
	int 0x15
	
	;; If carry flag or ah not 0 => FAIL,	
	jc .a20_fail
	test ah,0
	jnz .a20_fail

	;; Else OK
	jmp .a20_success
	
.a20_fail:
	mov si,str_a20_fail
	call printstr
	xchg bx,bx
	
	mov ax,0
	ret
	
.a20_success:
	mov ax,1
	ret
