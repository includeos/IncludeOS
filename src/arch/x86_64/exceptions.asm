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
[BITS 64]
extern __cpu_exception

SECTION .bss
__amd64_registers:
    resb 8*24

%macro CPU_EXCEPT 1
global __cpu_except_%1:function
__cpu_except_%1:
    call save_cpu_regs

    ;; reveal origin stack frame
    push rbp
    mov  rbp, rsp
    ;; re-align stack
    and rsp, ~0xF
    ;; enter panic
    mov rdi, __amd64_registers
    mov rsi, %1
    mov rdx, 0
    call __cpu_exception
%endmacro

%macro CPU_EXCEPT_CODE 1
global __cpu_except_%1:function
__cpu_except_%1:
    call save_cpu_regs

    ;; pop error code
    pop rdx
    ;; reveal origin stack frame
    push rbp
    mov  rbp, rsp
    ;; re-align stack
    and rsp, ~0xF
    ;; enter panic
    mov rdi, __amd64_registers
    mov rsi, %1
    call __cpu_exception
%endmacro

SECTION .text
%define regs(r) [__amd64_registers + r]
save_cpu_regs:
    mov regs( 0), rax
    mov regs( 8), rbx
    mov regs(16), rcx
    mov regs(24), rdx
    mov regs(32), rbp

    mov regs(40), r8
    mov regs(48), r9
    mov regs(56), r10
    mov regs(64), r11
    mov regs(72), r12
    mov regs(80), r13
    mov regs(88), r14
    mov regs(96), r15

    mov rax, QWORD [rsp + 32]
    mov regs(104), rax
    mov regs(112), rsi
    mov regs(120), rdi
    mov rax, QWORD [rsp + 8]
    mov regs(128), rax

    pushf
    pop QWORD regs(136)
    mov rax, cr0
    mov regs(144), rax
    mov QWORD regs(152), 0
    mov rbx, cr2
    mov regs(160), rbx
    mov rcx, cr3
    mov regs(168), rcx
    mov rdx, cr4
    mov regs(176), rdx
    mov rax, cr8
    mov regs(184), rax
    ret

CPU_EXCEPT 0
CPU_EXCEPT 1
CPU_EXCEPT 2
CPU_EXCEPT 3
CPU_EXCEPT 4
CPU_EXCEPT 5
CPU_EXCEPT 6
CPU_EXCEPT 7
CPU_EXCEPT_CODE 8
CPU_EXCEPT 9
CPU_EXCEPT_CODE 10
CPU_EXCEPT_CODE 11
CPU_EXCEPT_CODE 12
CPU_EXCEPT_CODE 13
CPU_EXCEPT_CODE 14
CPU_EXCEPT 15
CPU_EXCEPT 16
CPU_EXCEPT_CODE 17
CPU_EXCEPT 18
CPU_EXCEPT 19
CPU_EXCEPT 20
CPU_EXCEPT_CODE 30
