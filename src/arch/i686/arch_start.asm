
global __arch_start:function
global fast_kernel_start:function
extern kernel_start

[BITS 32]

;; @param: eax - multiboot magic
;; @param: ebx - multiboot bootinfo addr
__arch_start:

    ;; Create stack frame for backtrace
    push ebp
    mov ebp, esp

    ;; hack to avoid stack protector
    mov DWORD [0x1014], 0x89abcdef

fast_kernel_start:
    ;; Push params on 16-byte aligned stack
    sub esp, 8
    and esp, -16
    mov [esp], eax
    mov [esp+4], ebx

    call kernel_start

    ;; Restore stack frame
    mov esp, ebp
    pop ebp
    ret
