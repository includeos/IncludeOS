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
    
    ; disable SSE
    mov eax, cr0
    ;or  ax, ~0xFFFB  ;set coprocessor emulation CR0.EM
    and ax, ~0x2     ;clear coprocessor monitoring  CR0.MP
    mov cr0, eax
  
    mov eax, cr4
    and  ax, ~(3 << 9)
    mov cr4, eax
    
    ; attempt to use SSE
    movsd xmm0, qword [0x0]
    ; should never reach here
    hlt

reset_idtr:
    dw      400h - 1
    dd      0
