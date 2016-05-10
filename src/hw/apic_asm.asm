USE32
global apic_enable
global spurious_intr

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

spurious_intr:
    iret
