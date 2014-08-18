_start:
	jmp boot

srv_size:
	.word 0
	.word 0
boot:
	mov '!',%al
	call print_al
	jmp boot

print_al:
	xor %edx,%edx
	mov 1,%ah
	int $0x14
	ret
	
