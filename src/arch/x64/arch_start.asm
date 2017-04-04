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

[BITS 32]
__arch_start:
    ;; disable old paging
    mov eax, cr0                                   ; Set the A-register to control register 0.
    and eax, 01111111111111111111111111111111b     ; Clear the PG-bit, which is bit 31.
    mov cr0, eax                                   ; Set control register 0 to the A-register.
    ;; clear page tables
    mov edi, 0x9000    ; Set the destination index to 0x9000.
    mov cr3, edi       ; Set control register 3 to the destination index.
    xor eax, eax       ; Nullify the A-register.
    mov ecx, 4096      ; Set the C-register to 4096.
    rep stosd          ; Clear the memory.

    ;; create page directory
    mov ecx, 512    ;; entries
    mov edi, cr3
    mov ebx, 0xA003
  .ptd_loop:
    mov DWORD [edi], ebx
    add ebx, 0x1000
    add edi, 8
    loop .ptd_loop

    ;; create page tables
    mov ecx, 1048576 ;; num pages
    mov edi, 0xA000
    mov ebx, 0x0003  ;; present + write
  .pt_loop:
    mov DWORD [edi], ebx
    add ebx, 0x1000
    add edi, 8
    loop .pt_loop

    ;; enable long mode
    mov ecx, 0xC0000080          ; EFER MSR
    rdmsr
    or eax, 1 << 8               ; LM-bit (bit 8)
    wrmsr

    ;; enable paging & protected mode
    mov eax, cr0                 ; Set the A-register to control register 0.
    or  eax, 1 << 31 | 1 << 0    ; Set the PG-bit, which is the 31nd bit, and the PM-bit, which is the 0th bit.
    mov cr0, eax                 ; Set control register 0 to the A-register.
    ;; load 64-bit GDT
    lgdt [GDT64.Pointer]
    jmp  GDT64.Code:long_mode

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
    ;; geronimo
    call kernel_start
    ret

GDT64:                           ; Global Descriptor Table (64-bit).
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
