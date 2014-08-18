USE16
	;; Memory layout, 16-bit
	%define _boot_segment 0x7c0 
	%define _os_segment 0x800
	%define _os_pointer 0x0

	;; Memory layout, 32-bit
	%define _mode32_code_segment 0x08
	%define _mode32_data_segment 0x10

	%define _kernel_loc 0x8000
	%define _kernel_stack 0x80000
	
	;; We don't really need a stack, except for calls
	%define _stack_segment 0x7000
	%define _stack_pointer 0xfffe ;Within the ss, for 16 bit

	;; Char helpers
	%define _CR 0x0D
	%define _LF 0x0A

	;; ELF offset of _start in text section
	%define _elf_start 0x34
	
;;; START
;;; We'll start at the beginning, but jump over some data
_start:
	jmp boot

;;; The size of the service on disk, to be loaded at boot. (Convenient to 
;;; have it at the beginning, so it remains at a fixed location. )
srv_size:
	dd 0			

srv_offs:
	dd 0
;;; Actual start
boot:
	xchg bx,bx
	;; Need to set data segment, to access strings
	mov ax, _boot_segment	
	mov ds, ax		

	;; Verify the offsets (for debugging)
	mov ax,[srv_size]
	mov bx,[srv_offs]

	;; Print bootloader string
	mov si, str_boot
	call printstr 
	
	;; Load os from disk, enter protected mode, and jump	
	call load_os
	call protected_mode	
	call start_os
	call never
	
protected_mode:
	;xchg bx,bx
	cli
	;; Load global descriptor table register
	lgdt [gdtr] ;;Bochs seems to allready have one
	;; Set the 2n'd bit in cr0
	mov eax,cr0
	or al,1
	mov cr0,eax
	
	;xchg bx,bx
	;; 	jmp 0x8:$+3
	jmp _mode32_code_segment:mode32+(_boot_segment<<4)

USE16

load_os:
	;xchg bx,bx	
	mov bx,_os_segment
	mov es,bx
	mov bx,_os_pointer

	mov ch,0
	mov cl,2		;Sector to start load
	mov dh,0		;Which head
	mov dl,0x80		;Which disk
	mov al,[srv_size]	;Size (Now max 256 sectors)
	mov ah,2

	int 0x13
	cmp ah,0
	je success

read_error:
	mov si,str_err_disk
	call printstr
	ret

success:
	mov si,str_success
	call printstr
	ret
	
start_os:
	ret


;;; Should never get here
never:
	;; 	sti
	cli
	hlt
	mov eax,0xf0010000	;"Fool"
	jmp never
		
	
;;; 16-bit code
USE16
	
printstr:
	mov ah,0eh
.repeat:	
	lodsb
	cmp al,0
	je .done
	int 10h
	jmp .repeat
.done:
	ret
	
;;; Print the char in %al to serial port, via BIOS int.
;;; OBS: Only works in real mode.
;;; OBS: In Bochs, looks like serial port must be initialized first
print_al_serial:
	xor edx,edx
	mov ah,1
	int 14h
	ret
	
print_al_scr:
	mov ah,0x0e
	int 0x10
	
;;; DATA 
str_boot:
	db `IncludeOS Booting...\n\r`,0

str_success:
	db `Service successfully loaded - starting\n\r`,0

str_mode32:
	db `Now in 32-bit mode...\n\r`,0

str_err_disk:
	db `Disk read error\n\r`,0

USE32
ALIGN 32
;; Global descriptor table
gdtr:
	dw gdt32_end -gdt32 -1
	dq gdt32+(_boot_segment<<4)
gdt32:
	;; Entry 0x0: Null desriptor
	dq 0x0 
	;; Entry 0x8: Code segment
	dw 0xffff		       ;Limit
	dw 0x0000		       ;Base 15:00
	db 0x00			       ;Base 23:16
	dw 0xc09a		       ;Flags
	db 0x00			       ;Base 32:24
	;; Entry 0x10: Data segment
	dw 0xffff		       ;Limit
	dw 0x0000		       ;Base 15:00
	db 0x00			       ;Base 23:16
	dw 0xc092		       ;Flags
	db 0x00			       ;Base 32:24	
gdt32_end:
	db `32`
;;; GDT done

;;; 32-bit code
USE32				
ALIGN 32
mode32:


	mov eax,0x123abc00
	;; 	mov si,mode32_string
	;; 	call printstr
	;; sti ;Only after installing an IDT.
	
	;; Compute service address (kernel entry + elf-offset)
	;; Putting this in ecx... Good idea? Don't know.
	;; TODO: Check gnu calling conventions to see if ecx is preserved
	xor ecx,ecx
	mov ecx,[srv_offs]
	add ecx,_kernel_loc
	
	;; Set up 32-bit data segment	
	mov eax,_mode32_data_segment
	;; Set up stack
	mov ss,eax
	mov esp,_kernel_stack
	;; mov esi,_kernel_stack 	;Was e00c3
	mov ebp,_kernel_stack

	
	;; Set up data segment
	mov ds,eax
	mov es,eax
	mov fs,eax
	mov gs,eax
	
	xchg bx,bx
	
	;; Jump to service
	jmp ecx
	
	;; NEVER
	jmp mode32

;; BOOT SIGNATURE
	times 510-($-$$) db 0	;
	dw 0xAA55
