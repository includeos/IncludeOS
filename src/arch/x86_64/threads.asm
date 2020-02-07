global __clone:function
global __clone_return:function
global __migrate_resume:function
global __thread_yield:function
global __thread_restore:function
extern __thread_suspend_and_yield

%macro PUSHAQ 0
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
%endmacro
%macro POPAQ 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
%endmacro

SECTION .text
__clone:
    ;; RDI = func
    ;; RSI = stack
    ;; RDX = flags
    ;; RCX = argument
    ;; R8  = ptid
    ;; R9  = tls
    ;; ctid (on stack)
    mov r10, [rsp+8] ;; ctid
    PUSHAQ
    sub rsp, 8 ;; space for return value
    mov r11, rsp

    ;; put function and argument on stack
    sub rsi, 0x10
    mov [rsi+0], rcx
    mov [rsi+8], rdi
    push rsi ;; stack
    push rdi ;; function

    push r11 ;; old stack
    push r10 ;; ctid

    extern clone_helper
    call clone_helper
    ;; cleanup
    add rsp, 0x18

    ;; restore some regs
    pop rsp ;; RSI: stack

__clone_resume:
    ;; call function with argument
    pop rdi
    pop rcx
    and rsp, -16
    call QWORD rcx

    ;; exit value in rax
    mov rdi, rax
    extern exit
    call exit

__migrate_resume:
    mov rsp, rdi
    jmp __clone_resume

__clone_return:
    mov rsp, rdi
    pop rax ;; restore thread id
    POPAQ
    ;; return to pthread_create
    ret

__thread_yield:
    ;; a normal function call
    ;; preserve callee-saved regs
    ;; RBX, RBP, and R12–R15
    PUSHAQ
    ;; now save this thread
    mov rdi, rsp ;; my stack
    ;; align stack
    sub rsp, 8
    call __thread_suspend_and_yield
	;; restore early (no yield happened)
	add rsp, 8
    ;; restore saved registers
    POPAQ
    ret

__thread_restore:
    mov rsp, rdi
    ;; restore saved registers
    POPAQ
    ret
