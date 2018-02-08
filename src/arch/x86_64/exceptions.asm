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
%define RAX 0
%define RBX 1
%define RCX 2
%define RDX 3
%define RBP 4
%define R8  5
%define R9  6
%define R10 7
%define R11 8
%define R12 9
%define R13 10
%define R14 11
%define R15 12
%define RSP 13
%define RSI 14
%define RDI 15
%define RIP 16
%define FLA 17
%define CR0 18
%define CR1 19
%define CR2 20
%define CR3 21
%define CR4 22
%define CR8 23


%define regs(r) [__amd64_registers + r * 8]


;; We call save_cpu regs, creating another frame
%define SZ_CALL_FRAME 8

;; Exception handler stack (no privilege change)
;; Intel manual Vol. 3A §6-13
%define ERRCODE_OFFS SZ_CALL_FRAME
%define EIP_OFFS SZ_CALL_FRAME + 8
%define CS_OFFS EIP_OFFS + 8
%define FLAGS_OFFS CS_OFFS + 8

;; We don't have the previous stack pointer on stack
;; Make a best guess
%define RSP_OFFS SZ_CALL_FRAME * 2 + ERRCODE_OFFS

save_cpu_regs:
    mov regs(RAX), rax
    mov regs(RBX), rbx
    mov regs(RCX), rcx
    mov regs(RDX), rdx
    mov regs(RBP), rbp

    mov regs(R8), r8
    mov regs(R9), r9
    mov regs(R10), r10
    mov regs(R11), r11
    mov regs(R12), r12
    mov regs(R13), r13
    mov regs(R14), r14
    mov regs(R15), r15

    mov rax, rsp
    add rax, RSP_OFFS
    mov regs(RSP), rax
    mov regs(RSI), rsi
    mov regs(RDI), rdi
    mov rax, QWORD [rsp + EIP_OFFS]
    mov regs(RIP), rax

    mov rax, QWORD [rsp + FLAGS_OFFS]
    mov regs(FLA),rax
    mov rax, cr0
    mov regs(CR0), rax
    mov QWORD regs(CR1), 0
    mov rbx, cr2
    mov regs(CR2), rbx
    mov rcx, cr3
    mov regs(CR3), rcx
    mov rdx, cr4
    mov regs(CR4), rdx
    mov rax, cr8
    mov regs(CR8), rax
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
