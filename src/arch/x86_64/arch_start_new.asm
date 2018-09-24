; This file is a part of the IncludeOS unikernel - www.includeos.org
;
; Copyright 2015 Oslo and Akershus University College of Applied Sciences
; and Alfred Bratterud
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.
global __arch_start:function
global __gdt64_base_pointer
extern kernel_start
extern __multiboot_magic
extern __multiboot_addr

%define P4_TAB             0x1000
%define P3_TAB             0x2000
%define NUM_ENTRIES        512
%define STACK_LOCATION     0x900000

;; CR0 paging enable bit
%define PAGING_ENABLE 0x80000000
;; CR0 Supervisor write-protect enable
%define SUPER_WP_ENABLE 0x10000

;; Extended Feature Enable Register (MSR)
%define IA32_EFER_MSR 0xC0000080
;; EFER Longmode bit
%define LONGMODE_ENABLE 0x100
;; EFER Execute Disable bit
%define NX_ENABLE 0x800


[BITS 32]
__arch_start:
    ;; disable old paging
    mov eax, cr0
    and eax, ~PAGING_ENABLE  ;; clear PG (bit 31)
    mov cr0, eax

    ;; address for Page Map Level 4
    mov edi, P4_TAB
    mov cr3, edi

    ;; clear out P4 and P3
    mov ecx, 0x2000 / 0x4
    xor eax, eax       ; Nullify the A-register.
    rep stosd

    ;; create page map entry
    mov edi, P4_TAB
    mov DWORD [edi], P3_TAB | 0x3 ;; present+write

    ;; create 512x 1GB mappings
    mov ecx, NUM_ENTRIES
    mov edi, P3_TAB
    mov eax, 0x0 | 0x3 | 1 << 7 ;; present + write + huge
    mov ebx, 0x0

.ptd_loop:
    mov DWORD [edi],   eax   ;; Low word
    mov DWORD [edi+4], ebx   ;; High word
    add eax, 1 << 30         ;; 1GB increments
    adc ebx, 0               ;; increment high word when CF set
    add edi, 8
    loop .ptd_loop

    ;; enable PAE
    mov eax, cr4
    or  eax, 1 << 5
    mov cr4, eax

    ;; enable long mode
    mov ecx, IA32_EFER_MSR
    rdmsr
    or  eax, (LONGMODE_ENABLE | NX_ENABLE)

    wrmsr

    ;; enable paging
    mov eax, cr0                 ; Set the A-register to control register 0.
    or  eax, (PAGING_ENABLE | SUPER_WP_ENABLE)
    mov cr0, eax                 ; Set control register 0 to the A-register.

    ;; load 64-bit GDT
    lgdt [__gdt64_base_pointer]
    jmp  GDT64.Code:long_mode


[BITS 64]
long_mode:
    cli

    ;; segment regs
    mov cx, GDT64.Data
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx
    mov ss, cx

    ;; set up new stack for 64-bit
    push rsp
    mov  rsp, STACK_LOCATION
    mov  rbp, rsp

    ;; setup temporary smp table
    mov rax, sentinel_table
    mov rdx, 0
    mov rcx, 0xC0000100 ;; FS BASE
    wrmsr

    ;; geronimo!
    mov  edi, DWORD[__multiboot_magic]
    mov  esi, DWORD[__multiboot_addr]
    call kernel_start
    pop  rsp
    ret

sentinel_table:
    dq sentinel_table ;; 0x0
    dq 0 ;; 0x8
    dq 0 ;; 0x10
    dq 0 ;; 0x18
    dq 0 ;; 0x20
    dq 0x123456789ABCDEF

SECTION .data
GDT64:
  .Null: equ $ - GDT64         ; The null descriptor.
    dq 0
  .Code: equ $ - GDT64         ; The code descriptor.
    dw 0                         ; Limit (low).
    dw 0                         ; Base (low).
    db 0                         ; Base (middle)
    db 10011010b                 ; Access (exec/read).
    db 00100000b                 ; Granularity.
    db 0                         ; Base (high).
  .Data: equ $ - GDT64         ; The data descriptor.
    dw 0                         ; Limit (low).
    dw 0                         ; Base (low).
    db 0                         ; Base (middle)
    db 10010010b                 ; Access (read/write).
    db 00000000b                 ; Granularity.
    db 0                         ; Base (high).
  .Task: equ $ - GDT64         ; TSS descriptor.
    dq 0
    dq 0

    dw 0x0 ;; alignment padding
__gdt64_base_pointer:
    dw $ - GDT64 - 1             ; Limit.
    dq GDT64                     ; Base.
