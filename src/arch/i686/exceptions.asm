[BITS 32]
extern __cpu_exception

SECTION .bss
i386_registers:
  resb 4*16

%macro CPU_EXCEPT 1
global __cpu_except_%1:function
__cpu_except_%1:
    call save_cpu_regs

    ;; new stack frame
    push ebp
    mov  ebp, esp
    ;; enter panic
    push 0
    push %1
    push i386_registers
    call __cpu_exception
%endmacro

%macro CPU_EXCEPT_CODE 1
global __cpu_except_%1:function
__cpu_except_%1:
    call save_cpu_regs

    ;; pop error code
    pop edx
    ;; new stack frame
    push ebp
    mov  ebp, esp
    ;; enter panic
    push edx
    push %1
    push i386_registers
    call __cpu_exception
%endmacro

SECTION .text
%define regs(r) [i386_registers + r]
save_cpu_regs:
    mov regs( 0), eax
    mov regs( 4), ebx
    mov regs( 8), ecx
    mov regs(12), edx

    mov regs(16), ebp
    mov eax, [esp + 16] ;; esp
    mov regs(20), eax
    mov regs(24), esi
    mov regs(28), edi

    mov eax, [esp + 4] ;; eip
    mov regs(32), eax
    pushf ;; eflags
    pop DWORD regs(36)
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
