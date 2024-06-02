global __syscall_entry:function
extern syscall_entry

SECTION .text
__syscall_entry:
    ;; store return address
    push rcx
    ;;  Convert back from syscall conventions
    ;;  Last param on stack movq 8(rsp),r9
    mov r9, r8
    mov r8, r10
    mov rcx, rdx
    mov rdx, rsi
    mov rsi, rdi
    mov rdi, rax
    call syscall_entry
    ;; return to rcx
    pop rcx
    jmp QWORD rcx
