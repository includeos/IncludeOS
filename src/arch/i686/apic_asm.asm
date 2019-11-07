USE32
global spurious_intr:function
global lapic_send_eoi:function
global get_cpu_id:function
global get_cpu_eip:function
global get_cpu_esp:function
global reboot_os:function

section .text
get_cpu_id:
    mov eax, [fs:0x0]
    ret

get_cpu_esp:
    mov eax, esp
    ret

get_cpu_eip:
    mov eax, [esp]
    ret

spurious_intr:
    iret

lapic_send_eoi:
    mov eax, 0xfee000B0
    mov DWORD [eax], 0
    ret

reboot_os:
    ; load bogus IDT
    lidt [reset_idtr]
    ; 1-byte breakpoint instruction
    int3

reset_idtr:
    dw      400h - 1
    dd      0
