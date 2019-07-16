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

global __thread_yield:function
global __thread_restore:function
extern __thread_self_store

;; x86_64 / System V ABI calling convention
%define arg0 rax
%define arg1 rdi
%define arg2 rsi
%define arg3 rdx
%define arg4 rcx
%define arg5 r8
%define arg6 r9

SECTION .text
__thread_yield:
    ;; a normal function call
    ;; preserve callee-saved regs
    ;; RBX, RBP, and R12–R15
    ;; as well as some thread-specific values
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    ;; now save this thread
    mov rdi, next_instruction
    mov rsi, rsp ;; my stack
    ;; align stack
    sub rsp, 8
    call __thread_self_store
    ret

__thread_restore:
    mov rsp, rsi
    jmp next_instruction

next_instruction:
    ;; restore saved registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret
