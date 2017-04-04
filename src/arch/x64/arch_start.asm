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
extern kernel_start

%define PAGE_SIZE          0x1000
%define PAGE_MAP_TAB       0x1000
%define PAGE_DIR_PTR_TAB   0x2000
%define PAGE_DIR_TAB       0x3000
%define PAGE_TABLE        0x10000
%define STACK_LOCATION    0xA0000

[BITS 32]
__arch_start:
    ;; disable old paging
    mov eax, cr0                                   ; Set the A-register to control register 0.
    and eax, 01111111111111111111111111111111b     ; Clear the PG-bit, which is bit 31.
    mov cr0, eax                                   ; Set control register 0 to the A-register.
    ;; address for Page Map Level 4
    mov edi, PAGE_MAP_TAB
    mov cr3, edi
    ;; clear page directory pointer table
    ;; ... and page directory table
    mov ecx, 0x3000    ; clear 3 pages
    xor eax, eax       ; Nullify the A-register.
    rep stosd

    ;; create page map entry
    mov edi, PAGE_MAP_TAB
    mov DWORD [edi], PAGE_DIR_PTR_TAB | 0x3 ;; present+write

    ;; create page directory pointer table entry
    mov edi, PAGE_DIR_PTR_TAB
    mov DWORD [edi], PAGE_DIR_TAB | 0x3 ;; present+write

    ;; create page directory entries
    mov ecx, 512     ;; num entries
    mov edi, PAGE_DIR_TAB
    mov ebx, PAGE_TABLE | 0x3 ;; pagetables first address + present+write
  .ptd_loop:
    mov DWORD [edi], ebx
    add ebx, PAGE_SIZE
    add edi, 8
    loop .ptd_loop

    ;; create page table entries
    mov ecx, 1048576 ;; num pages
    mov edi, PAGE_TABLE
    mov ebx, 0x3  ;; present + write
  .pt_loop:
    mov DWORD [edi], ebx
    add ebx, PAGE_SIZE
    add edi, 8
    loop .pt_loop

    ;; enable PAE
    mov eax, cr4
    or  eax, 1 << 5
    mov cr4, eax

    ;; enable long mode
    mov ecx, 0xC0000080          ; EFER MSR
    rdmsr
    or eax, 1 << 8               ; Long Mode bit
    wrmsr

    ;; enable paging & protected mode
    mov eax, cr0                 ; Set the A-register to control register 0.
    or  eax, 1 << 31 | 1 << 0    ; Set the PG-bit, which is the 31nd bit, and the PM-bit, which is the 0th bit.
    mov cr0, eax                 ; Set control register 0 to the A-register.

    ;; load 64-bit GDT
    lgdt [GDT64.Pointer]

    jmp  GDT64.Code:long_mode


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
    dw 0x0 ;; alignment padding
  .Pointer:                    ; The GDT-pointer.
    dw $ - GDT64 - 1             ; Limit.
    dq GDT64                     ; Base.

[BITS 64]
long_mode:
    cli
    ;; segment regs
    mov ax, GDT64.Data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ;; set up new stack for 64-bit
    push rsp
    mov rsp, STACK_LOCATION
    mov rbp, rsp

    ;; geronimo!
    call kernel_start
    pop  rsp
    ret
