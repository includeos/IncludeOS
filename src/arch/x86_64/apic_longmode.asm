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
;
global __apic_trampoline:function
extern __gdt64_base_pointer
extern revenant_main

%define P4_TAB    0x1000

[BITS 32]
__apic_trampoline:
    pop edi  ;; cpuid

    ;; use same pagetable as CPU 0
    mov eax, P4_TAB
    mov cr3, eax

    ;; enable PAE
    mov eax, cr4
    or  eax, 1 << 5
    mov cr4, eax

    ;; enable long mode
    mov ecx, 0xC0000080          ; EFER MSR
    rdmsr
    or  eax, 1 << 8              ; Long Mode bit
    wrmsr

    ;; enable paging
    mov eax, cr0                 ; Set the A-register to control register 0.
    or  eax, 1 << 31
    mov cr0, eax                 ; Set control register 0 to the A-register.

    ;; load 64-bit GDT
    lgdt [__gdt64_base_pointer]
    jmp  0x8:long_mode ;; 0x8 = code seg

[BITS 64]
long_mode:
    ;; segment regs
    mov cx, 0x10 ;; 0x10 = data seg
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx
    mov ss, cx

    ;; align stack
    and  rsp, -16
    ;; retrieve CPU id
    mov rax, 1
    cpuid
    shr rbx, 24
    ;; geronimo!
    mov rdi, rbx
    call revenant_main
    ; stop execution
    cli
    hlt
