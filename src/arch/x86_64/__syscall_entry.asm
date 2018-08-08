;; This file is a part of the IncludeOS unikernel - www.includeos.org
;;
;; Copyright 2018 IncludeOS AS, Oslo, Norway
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

global __syscall_entry:function
global __test_syscall:function
extern syscall_entry
extern kprintf

;; x86_64 / System V ABI calling convention
%define arg1 rdi
%define arg2 rsi
%define arg3 rdx
%define arg4 rcx
%define arg5 r8
%define arg6 r9

;; Preserve caller-saved registers
%macro PUSHAQ 0
   push rax
   push rcx
   push rdx
   push rdi
   push rsi
   push r8
   push r9
   push r10
   push r11
   ; Preserve extended state
   ;fxsave [__xsave_storage_area]
%endmacro
%macro POPAQ 0
   ; Restore extended state
   ;fxrstor [__xsave_storage_area]

   pop r11
   pop r10
   pop r9
   pop r8
   pop rsi
   pop rdi
   pop rdx
   pop rcx
   pop rax
%endmacro

%define stack_size 32768

section .bss
temp_stack:
    resb stack_size
temp_old_stack:
    resq 1
temp_rcx:
    resq 1

section .text
__syscall_entry:
    ;; mov [temp_rcx], rcx
    ;;     mov [temp_old_stack], rsp
    ;;     mov rsp, temp_stack + stack_size - 16

    push rcx
    ;;     sub rsp, 8

    ;;  Convert back from syscall conventions
    ;;  Last param on stack movq 8(rsp),r9
    mov r9, r8
    mov r8, r10
    mov rcx, rdx
    mov rdx, rsi
    mov rsi, rdi
    mov rdi, rax

    call syscall_entry
    ;;     add rsp, 8
    pop rcx

    ;; mov rsp, [temp_old_stack]
    jmp QWORD rcx               ;[temp_rcx]


__test_syscall:
    mov [kparam.stack], rsp
    mov arg1, kparam.rsp
    mov arg2, [kparam.stack]
    mov arg3, 0

    push rsp
    and rsp, ~15
    call kprintf
    pop rsp

    mov rsi, rdi           ; shift for syscall
    mov edi, 0x1002         ;/* SET_FS register */
    mov eax, 158            ;/* set fs segment to */


    syscall                 ;/* arch_prctl(SET_FS, arg)*/

    mov [kparam.stack], rsp
    mov arg1, kparam.rsp
    mov arg2, [kparam.stack]
    mov arg3, 0

    push rsp
    and rsp, ~15
    call kprintf
    pop rsp
    ret

;; Make thread local
kparam:
    .rsp:
    db `__test_syscall: Stack: 0x%lx\n`,0
    .fmt_rcx:
    db `__test_syscall: rcx: 0x%lx\n`,0
    .stack:
    dq 0
    .rcx:
    dq 0
