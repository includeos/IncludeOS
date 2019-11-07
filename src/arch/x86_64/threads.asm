global __thread_yield:function
global __thread_restore:function
extern __thread_suspend_and_yield

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
    mov rdi, __thread_restore
    mov rsi, rsp ;; my stack
    ;; align stack
    sub rsp, 8
    call __thread_suspend_and_yield

__thread_restore:
    mov rsp, rsi
    ;; restore saved registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret
