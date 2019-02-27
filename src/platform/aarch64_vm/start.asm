/*;; This file is a part of the IncludeOS unikernel - www.includeos.org
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
*/

//USE32
.extern __arch_start
.extern __serial_print1
.global __multiboot_magic
.global __multiboot_addr
.global _start
.global __xsave_enabled
.global __avx_enabled

.set MB_MAGIC,  0x1BADB002
.set MB_FLAGS,  0x3  //;; ALIGN + MEMINFO

//;; stack base address at EBDA border
//;; NOTE: Multiboot can use 9d400 to 9ffff
.set  STACK_LOCATION, 0x9D3F0

.extern _MULTIBOOT_START_
.extern _LOAD_START_
.extern _LOAD_END_
.extern _end
.extern fast_kernel_start

.align 4
.section .multiboot
  dd  MB_MAGIC
  dd  MB_FLAGS
  dd  -(MB_MAGIC + MB_FLAGS)
  dd _MULTIBOOT_START_
  dd _LOAD_START_
  dd _LOAD_END_
  dd _end
  dd _start
//  ;; used for faster live updates
  dd 0xFEE1DEAD
  dd fast_kernel_start

.set data_segment 0x10
.set code_segment 0x08

.section .data
__xsave_enabled:
    dw 0x0
__avx_enabled:
    dw 0x0
__multiboot_magic:
    dd 0x0
__multiboot_addr:
    dd 0x0

section .text
;; Multiboot places boot paramters on eax and ebx.
_start:
  ;; load simple GDT
  lgdt [gdtr]

  ;; Reload all data segments to new GDT
  jmp code_segment:rock_bottom

;; Set up stack and load segment registers
;; (e.g. this is the very bottom of the stack)
rock_bottom:
  mov cx, data_segment
  mov ss, cx
  mov ds, cx
  mov es, cx
  mov fs, cx
  mov cx, 0x18 ;; GS segment
  mov gs, cx

  ;; 32-bit stack ptr
  mov esp, STACK_LOCATION
  mov ebp, esp

  ;; enable SSE before we enter C/C++ land
  call enable_sse
  ;; Enable modern x87 FPU exception handling
  call enable_fpu_native
  ;; try to enable XSAVE before checking AVX
  call enable_xsave
  ;; enable AVX if xsave and avx supported on CPU
  call enable_avx

  ;;  Save multiboot params
  mov DWORD [__multiboot_magic], eax
  mov DWORD [__multiboot_addr], ebx

  call __arch_start
  jmp __start_panic
//TODO ARM
enable_fpu_native:
    push eax
    mov eax, cr0
    or eax, 0x20
    mov cr0, eax
    pop eax
    ret
//TODO ENABLE NEON

enable_sse:
  push eax        ;preserve eax for multiboot
  mov eax, cr0
  and ax, 0xFFFB  ;clear coprocessor emulation CR0.EM
  or  ax, 0x2     ;set coprocessor monitoring  CR0.MP
  mov cr0, eax
  mov eax, cr4
  or ax, 3 << 9   ;set CR4.OSFXSR and CR4.OSXMMEXCPT at the same time
  mov cr4, eax
  pop eax
  ret

enable_xsave:
  push eax
  push ebx
  ; check for XSAVE support
  mov eax, 1
  xor ecx, ecx
  cpuid
  ; bit 26 ecx
  and ecx, 0x04000000
  cmp ecx, 0x04000000
  jne xsave_not_supported
  ; enable XSAVE
  mov eax, cr4
  or  eax, 0x40000
  mov cr4, eax
  mov WORD [__xsave_enabled], 0x1
xsave_not_supported:
  pop ebx
  pop eax
  ret

enable_avx:
  push eax
  push ebx
  ;; assuming cpuid with eax=1 supported
  mov eax, 1
  xor ecx, ecx
  cpuid
  ;; check bits 27, 28 (xsave, avx)
  and ecx, 0x18000000
  cmp ecx, 0x18000000
  jne avx_not_supported
  ;; enable AVX support
  xor ecx, ecx
  xgetbv
  or eax, 0x7
  xsetbv
  mov WORD [__avx_enabled], 0x1
avx_not_supported:
  pop ebx
  pop eax
  ret

__start_panic:
    sub esp, 4
    and esp, -16
    mov DWORD [esp], str.panic
    call __serial_print1
    cli
    hlt

ALIGN 32
gdtr:
  dw gdt32_end - gdt32 - 1
  dd gdt32
ALIGN 32
gdt32:
  ;; Entry 0x0: Null descriptor
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
  ;; Entry 0x18: GS Data segment
  dw 0x0100          ;Limit
  dw 0x1000          ;Base 15:00
  db 0x00            ;Base 23:16
  dw 0x4092          ;Flags / Limit / Type [F,L,F,Type]
  db 0x00            ;Base 32:24
gdt32_end:


str:
    .panic:
    db `Panic: OS returned to x86 start.asm. Halting\n`,0x0
