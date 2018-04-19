; This file is a part of the IncludeOS unikernel - www.includeos.org
;
; Copyright 2017 Oslo and Akershus University College of Applied Sciences
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
USE64

global __fiber_jumpstart
global __fiber_yield

extern fiber_jumpstarter


;; x86_64 / System V ABI calling convention
%define arg1 rdi
%define arg2 rsi
%define arg3 rdx
%define arg4 rcx
%define arg5 r8
%define arg6 r9

;; Preserve callee-saved registers
%macro PUSHEM 0
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
%endmacro

;; Restore callee-saved registers
%macro POPEM 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
%endmacro

;; Start a function in a new thread
;; @param 1: Stack pointer
;; @param 2: Delegate to call (via jumpstarter)
;; @param 3: Stack pointer save location
__fiber_jumpstart:

    ;;  Preserve caler saved regs
    PUSHEM

    ;;  Preserve old stack
    mov [arg3], rsp

    ;; Switch stack
    mov rsp, arg1

    ;; Push old stack loc. on new stack
    push arg3
    push rbp

    ;; Call jumpstarter with second  param
    mov arg1, arg2
    call fiber_jumpstarter

    ;; Restore saved stack
    pop rbp
    pop arg3
    mov rsp, [arg3]

    ;; Restore caller saved regs
    POPEM

    ret

;; Yield to a started or yielded thread
;; @param 1 : new stack to switch to
;; @param 2 : Stack pointer save location
__fiber_yield:

    ;;  Preserve caler saved regs
    PUSHEM

    ;; Update
    mov [arg2], rsp

    ;; Switch stack
    mov rsp, arg1

    ;; Restore caller saved regs
    POPEM

    ret
