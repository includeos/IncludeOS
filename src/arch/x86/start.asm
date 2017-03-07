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

%define data_segment 0x10
%define code_segment 0x08

section .text
;; Multiboot places boot paramters on eax and ebx.
_start:

  ;; With multiboot we get a possibly unusable GDT
  ;; So load our own
  lgdt [gdtr]

  ;; Reload all data segments to new GDT
  jmp code_segment:reload_segs
reload_segs:
  mov cx, data_segment
  mov ss, cx
  mov ds, cx
  mov es, cx
  mov fs, cx
  mov gs, cx

  ;; Set up stack.
  mov esp, 0xA0000
  mov ebp, esp

  ;; enable SSE before we enter C/C++ land
  call enable_sse

  ;;  Place multiboot parameters on stack
  push ebx
  push eax
  call kernel_start
  ret

enable_sse:
  push eax        ;preserve eax for multiboot
  mov eax, cr0
  and ax, 0xFFFB  ;clear coprocessor emulation CR0.EM
  or ax, 0x2      ;set coprocessor monitoring  CR0.MP
  mov cr0, eax
  mov eax, cr4
  or ax, 3 << 9   ;set CR4.OSFXSR and CR4.OSXMMEXCPT at the same time
  mov cr4, eax
  pop eax
  ret

ALIGN 32
gdtr:
  dw gdt32_end - gdt32 - 1
  dd gdt32
ALIGN 32
gdt32:
  ;; Entry 0x0: Null desriptor
  dq 0x0
  ;; Entry 0x8: Code segment
  dw 0xffff          ;Limit
  dw 0x0000          ;Base 15:00
  db 0x00            ;Base 23:16
  dw 0xcf9a          ;Flags / Limit / Type [F,L,F,Type]
  db 0x00            ;Base 32:24
  ;; Entry 0x10: Data segment
  dw 0xffff          ;Limit
  dw 0x0000          ;Base 15:00
  db 0x00            ;Base 23:16
  dw 0xcf92          ;Flags / Limit / Type [F,L,F,Type]
  db 0x00            ;Base 32:24
gdt32_end:
