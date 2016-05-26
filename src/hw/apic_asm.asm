USE32
global apic_enable
global spurious_intr
global bsp_lapic_send_eoi
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

bsp_lapic_send_eoi:
    push eax
    mov eax, 0xfee000B0
    mov DWORD [eax], 0
    pop eax
    ret

reboot:
    ; load bogus IDT
    lidt [reset_idtr]
    ; 1-byte breakpoint instruction
    int3

reset_idtr:
    dw      400h - 1
    dd      0
