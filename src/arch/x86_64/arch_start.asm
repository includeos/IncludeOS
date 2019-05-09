; This file is a part of the IncludeOS unikernel - www.includeos.org
;
; Copyright 2015 Oslo and Akershus University College of Applied Sciences
; and Alfred Bratterud
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
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
global fast_kernel_start:function
extern kernel_start
extern __multiboot_magic
extern __multiboot_addr

%define PAGE_SIZE               0x1000
%define P4_TAB                  0x1000 ;; one page
%define P3_TAB                  0x2000 ;; one page
%define P2_TAB                  0x100000 ;; many pages

%define IA32_EFER               0xC0000080
%define IA32_STAR               0xC0000081
%define IA32_LSTAR              0xc0000082
%define IA32_FMASK              0xc0000084
%define IA32_FS_BASE            0xc0000100
%define IA32_GS_BASE            0xc0000101
%define IA32_KERNEL_GS_BASE     0xc0000102

%define NUM_P3_ENTRIES     5
%define NUM_P2_ENTRIES     2560

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
;; EFER Syscall enable bit
%define SYSCALL_ENABLE 0x1


[BITS 32]
SECTION .text
__arch_start:
    ;; disable old paging
    mov eax, cr0
    and eax, 0x7fffffff  ;; clear PG (bit 31)
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

    ;; create 1GB mappings
    mov ecx, NUM_P3_ENTRIES
    mov edi, P3_TAB
    mov eax, P2_TAB | 0x3 ;; present + write
    mov ebx, 0x0

.p3_loop:
    mov DWORD [edi],   eax   ;; Low word
    mov DWORD [edi+4], ebx   ;; High word
    add eax, 1 << 12         ;; page increments
    adc ebx, 0               ;; increment high word when CF set
    add edi, 8
    loop .p3_loop

    ;; create 2MB mappings
    mov ecx, NUM_P2_ENTRIES
    mov edi, P2_TAB
    mov eax, 0x0 | 0x3 | 1 << 7 ;; present + write + huge
    mov ebx, 0x0

.p2_loop:
    mov DWORD [edi],   eax   ;; Low word
    mov DWORD [edi+4], ebx   ;; High word
    add eax, 1 << 21         ;; 2MB increments
    adc ebx, 0               ;; increment high word when CF set
    add edi, 8
    loop .p2_loop

    ;; enable PAE
    mov eax, cr4
    or  eax, 1 << 5
    mov cr4, eax

    ;; enable long mode
    mov ecx, IA32_EFER_MSR
    rdmsr
    or  eax, (LONGMODE_ENABLE | NX_ENABLE | SYSCALL_ENABLE)
    wrmsr

    ;; enable paging
    mov eax, cr0                 ; Set the A-register to control register 0.
    or  eax, (PAGING_ENABLE | SUPER_WP_ENABLE)
    mov cr0, eax                 ; Set control register 0 to the A-register.

    ;; load 64-bit GDT
    lgdt [__gdt64_base_pointer]
    jmp  GDT64.Code:long_mode


[BITS 64]
SECTION .text
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
    extern _ELF_START_
    push rsp
    mov  rsp, _ELF_START_
    push 0
    push 0
    mov  rbp, rsp

    mov ecx, IA32_STAR
    mov edx, 0x8
    mov eax, 0x0
    wrmsr

    ;; setup fake TLS table for SMP and SSP
    mov ecx, IA32_FS_BASE
    mov edx, 0x0
    mov eax, tls_table
    wrmsr

    mov ecx, IA32_GS_BASE
    mov edx, 0x0
    mov eax, smp_table
    wrmsr

    ;; geronimo!
    mov  edi, DWORD[__multiboot_magic]
    mov  esi, DWORD[__multiboot_addr]
    call kernel_start
    pop  rsp
    ret

;; this function can be jumped to directly from hotswap
fast_kernel_start:
    and  rsp, -16
    mov  edi, eax
    mov  esi, ebx
    call kernel_start
    cli
    hlt

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

SECTION .bss
tls_table:
    dq   tls_table
smp_table:
    resw 8
