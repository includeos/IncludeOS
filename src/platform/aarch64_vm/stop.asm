.global vm_exit
vm_exit:
    /* 0x20026 == ADP_Stopped_ApplicationExit */
    mov x1, #0x26
    movk x1, #2, lsl #16
    str x1, [sp,#0]

    /* Exit status code. Host QEMU process exits with that status. */
    mov x0, #0
    str x0, [sp,#8]

    /* x1 contains the address of parameter block.
     * Any memory address could be used. */
    mov x1, sp

    /* SYS_EXIT */
    mov w0, #0x18

    /* Do the semihosting call on A64. */
    hlt 0xf000
