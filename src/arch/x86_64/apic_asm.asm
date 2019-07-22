[BITS 64]
global spurious_intr:function
global lapic_send_eoi:function
global get_cpu_id:function
global get_cpu_eip:function
global get_cpu_esp:function
global get_cpu_ebp:function
global reboot_os:function
global __amd64_load_tr:function

get_cpu_id:
    mov rax, [gs:0x0]
    ret

get_cpu_esp:
    mov rax, rsp
    ret

get_cpu_ebp:
    mov rax, rbp
    ret

get_cpu_eip:
    mov rax, [rsp]
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
    dq      0

__amd64_load_tr:
    ltr di
    ret

GLOBAL intel_rdrand:function
intel_rdrand:
  mov eax, 0x1
retry_rdrand:
  rdrand rcx
  mov QWORD [rdi], rcx
  cmovb ecx, eax
  jae retry_rdrand
  cmp ecx, 0x1
  sete al
  ret

GLOBAL intel_rdseed:function
intel_rdseed:
  mov eax, 0x1
retry_rdseed:
  rdseed rcx
  mov QWORD [rdi], rcx
  cmovb ecx, eax
  jae retry_rdseed
  cmp ecx, 0x1
  sete al
  ret
