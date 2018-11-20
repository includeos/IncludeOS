;; This file is a part of the IncludeOS unikernel - www.includeos.org
;;
;; Copyright 2015 Oslo and Akershus University College of Applied Sciences
;; and Alfred Bratterud
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;;     http:;;www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;; See the License for the specific language governing permissions and
;; limitations under the License.

USE16
;; Memory layout, 16-bit
%define _boot_segment 0x07c0
%define _vga_segment  0xb800

;; Memory layout, 32-bit
%define _mode32_code_segment 0x08
%define _mode32_data_segment 0x10

%define _kernel_stack 0xA0000

;; We don't really need a stack, except for calls
%define _stack_segment 0x7000
%define _stack_pointer 0xfffe ;Within the ss, for 16 bit

;; Char helpers
%define _CR 0x0D
%define _LF 0x0A

;;; START
;;; We'll start at the beginning, but jump over some data
_start:
	jmp boot

;;; The size of the service on disk, to be loaded at boot. (Convenient to
;;; have it at the beginning, so it remains at a fixed location. )
ALIGN 4
srv_size:
	dd 0
srv_entry:
	dd 0
srv_load:
    dd 0

;;; Actual start
boot:
	;; Need to set data segment, to access strings
	mov ax, _boot_segment
	mov ds, ax

	; fast a20 enable
	in al, 0x92
	or al, 2
	out 0x92, al

	;;  Clear the screen
	call fill_screen

	;;  Print the IncludeOS logo
	mov esi, str_includeos
	call printstr
	mov ax, 0x07
	mov [color], ax
	mov esi, str_literally
	call printstr


  ; check that it was enabled
	test al, 0
	jz .a20_ok
	; NOT OK
	mov esi,str_a20_fail
	call printstr
	cli
	hlt
.a20_ok:
	call protected_mode

protected_mode:
	cli
	;; Load global descriptor table register
	lgdt [gdtr] ;;Bochs seems to allready have one
	;; Set the 1st bit in cr0
	mov eax, cr0
	or   al, 1
	mov cr0, eax
	jmp _mode32_code_segment:mode32+(_boot_segment<<4)

fill_screen:
	mov bx, _vga_segment
	mov es, bx
	mov bx, 0
	mov al, 0
	mov ah, [color]
.fill:
	mov [es:bx], ax
	add bx,2
	cmp bx, (25*80*2)
	jge .done
	jmp .fill
.done:
	ret

printstr:
	mov bx, _vga_segment
	mov es, bx
	mov bx, [cursor]
	mov ah, [color]
.repeat:
	lodsb
	cmp al,0
	je .done
	mov [es:bx], al
	mov [es:bx + 1], ah
	add bx, 2
	jmp .repeat
.done:
	mov [cursor], bx
	ret

str_includeos:
	db `#include <os> `,0
str_literally:
	db `\/\/ Literally `,0
str_a20_fail:
	db `A20 Err\n\r`,0
cursor:
	dw (80 * 11 * 2) + 48
color:
	dw 0x0d

USE32
ALIGN 4
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
	dw 0xcf9a		       ;Flags
	db 0x00			       ;Base 32:24
	;; Entry 0x10: Data segment
	dw 0xffff		       ;Limit
	dw 0x0000		       ;Base 15:00
	db 0x00			       ;Base 23:16
	dw 0xcf92		       ;Flags
	db 0x00			       ;Base 32:24
gdt32_end:
	db `32`
;;; GDT done

;;; 32-bit code
USE32
ALIGN 4
mode32:
	;; Set up 32-bit data segment
	mov eax,_mode32_data_segment
	;; Set up stack
	mov ss,eax
	mov esp,_kernel_stack
	;; mov esi,_kernel_stack 	;Was e00c3
	mov ebp,_kernel_stack

	;; Set up data segment
	mov ds, eax
	mov es, eax
	mov fs, eax
	mov gs, eax

	;; By default QEMU handles only 1 sector pr. read
	%define LOAD_SIZE 1

	;; Number of sectors to read for the entire service
  mov edx, [srv_size+(_boot_segment<<4)]
	mov eax, 1

	;; Location to load kernel
	mov edi,[srv_load+(_boot_segment<<4)]

.more:
	mov cl, LOAD_SIZE

	;; Load 1 sector from disk
	call ata_lba_read

	;; Increase LBA by 1 sector
	add eax, LOAD_SIZE

	;; Increase destination (in bytes)
	add edi, LOAD_SIZE * 512

	;; Decrement counter (srv_size) by 1 sector
	sub edx, LOAD_SIZE

	;; If all sectors loaded, move on, else get .more
	test edx, edx
	jnz .more  ;; jump when not zero

	;; Bochs breakpoint
	;;xchg bx,bx

	;; GERONIMO!
	;; Jump to service
	call DWORD [srv_entry+(_boot_segment<<4)]


	%include "boot/disk_read_lba.asm"

	;; BOOT SIGNATURE
	times 510-($-$$) db 0	;
	dw 0xAA55
