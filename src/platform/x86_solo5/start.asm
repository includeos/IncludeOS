
global get_fs_sentinel_value:function
global pre_initialize_tls:function
extern _ELF_START_
extern kernel_start
%define IA32_FS_BASE            0xc0000100

get_fs_sentinel_value:
    mov rax, [fs:0x28]
    ret

pre_initialize_tls:
    mov ecx, IA32_FS_BASE
    mov edx, 0x0
    mov eax, initial_tls_table
    wrmsr
    ;; stack starts at ELF boundary growing down
    mov rsp, _ELF_START_
    call kernel_start
    ret

SECTION .data
initial_tls_table:
    dd initial_tls_table
    dd 0
    dq 0
    dq 0
    dq 0
    dq 0
    dq 0x123456789ABCDEF
    dq 0x123456789ABCDEF
