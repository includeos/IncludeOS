USE16
	;; Memory layout, 16-bit
	%define _boot_segment 0x7c0 
	%define _os_segment 0x800
	%define _os_pointer 0x0

	;; Memory layout, 32-bit
	%define _mode32_code_segment 0x08
	%define _mode32_data_segment 0x10

	%define _kernel_loc 0x8000
	%define _kernel_stack 0x800000
	
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
%include "./asm/check_a20_16bit.asm"
;;; Actual start
boot:

	;; Need to set data segment, to access strings
	mov ax, _boot_segment	
	mov ds, ax		

	mov esi,str_boot
	call printstr
	
	call check_a20
	test al,0
	jz .a20_ok
	
	;; NOT OK
	mov esi,str_a20_fail
	call printstr	
	cli
	hlt
.a20_ok:
	mov esi,str_a20_ok
	call printstr

	
	
	call protected_mode	
	
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

	;; %include "asm/load_os_16bit.asm"
			
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

str_boot:
	db `IncludeOS!\n\r`,0
str_a20_ok:
	db `A20 OK!\n\r`,0
str_a20_fail:
	db `A20 NOT OK\n\r`,0

	
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

	
	;; Load LBA params
	%define CYL   0
	%define HEAD  0
	%define SECT  2
	%define HPC   1
	%define SPT  63
	
	;;  OBS: By default, Qemu handles only 1 sector pr. read	
	%define LOAD_SIZE 1 
	
	xor edx,edx
	
	
	mov edx,[srv_size+(_boot_segment<<4)]
	mov eax,1	;== ((CYL*HPC)+HEAD)*SPT+SECT-1		;

	
	;; Number of sectors to read (lets do one sector at a time)

	mov edi,_kernel_loc

.more:
	xor ecx, ecx
	mov cl,LOAD_SIZE

	;; Do the loading
	call ata_lba_read
	
	;; Increase LBA by 63 sectors
	add eax,LOAD_SIZE

	;; Increase destination by 63 sect. * sect.size
	add edi,LOAD_SIZE*512

	;; Decrement counter (srv_size) by load size
	sub edx,LOAD_SIZE

	;; If all sectors loaded, move on, else get .more
	cmp edx,0
	jge .more
	
	;; Compute service address (kernel entry + elf-offset)
	;; Putting this in ecx... Good idea? Don't know.
	;; TODO: Check gnu calling conventions to see if ecx is preserved
	xor eax,eax
	xor ebx,ebx
	xor ecx,ecx
	xor edx,edx
	xor edi,edi
	mov ecx,[srv_offs+(_boot_segment<<4)]
	;; add ecx,_kernel_loc // We've now placed the exact address in srv_offs.

	;; A20 test
	;; mov byte [0x10000],'!'
	

	;; GERONIMO!
	;; Jump to service
	xchg bx,bx
	;; 	hlt
	jmp ecx
	
	%include "asm/disk_read_lba.asm"
	
	
;; BOOT SIGNATURE
	times 510-($-$$) db 0	;
	dw 0xAA55
