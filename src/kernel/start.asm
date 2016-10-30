;; This file is a part of the IncludeOS unikernel - www.includeos.org
;;
;; Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

USE32
extern kernel_start
global _start

%define BOOT_SEGMENT 0x07c0

section .text
;; Multiboot places boot paramters on eax and ebx.
_start:
        ;; Stack base address to EMA boundary
        mov esp, 0xA0000
        mov ebp, esp

        ;; With multiboot we get a possibly unusable GDT
        ;; So load our own
        lgdt [gdtr]

        ;;  Place multiboot parameters on stack
        push ebx
        push eax
        call kernel_start

ALIGN 32
gdtr:
	dw gdt32_end - gdt32 - 1
	dd gdt32
ALIGN 32
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
