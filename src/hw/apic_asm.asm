USE32
global apic_enable
global spurious_intr
global get_cpu_id
global reboot

apic_enable:
    push ecx
    push eax
    mov			ecx, 1bh
    rdmsr
    bts			eax, 11
    wrmsr
    pop eax
    pop ecx
    ret

get_cpu_id:
    mov eax, 1
    cpuid
    shr ebx, 24
    mov eax, ebx
    ret

spurious_intr:
    iret

reboot:
    ; load bogus IDT
    lidt [reset_idtr]
    ; 1-byte breakpoint instruction
    int3

reset_idtr:
    dw      400h - 1
    dd      0
