/*
*/
.text
.global __arch_start
.global fast_kernel_start
.global init_serial

//linker handles this
//.extern kernel_start
//not sure if this is a sane stack location
.set STACK_LOCATION, 0x200000 - 16
.extern __stack_top
//__stack_top:

//;; @param: eax - multiboot magic
//;; @param: ebx - multiboot bootinfo addr
.align 8
__arch_start:
    //store any boot magic if present ?
    ldr x8,__boot_magic
    ldr x0,[x8]
    //
    mrs x0, s3_1_c15_c3_0
  //  ;; Create stack frame for backtrace

    ldr x30, =__stack_top
    mov sp, x30


  /*  ldr w0,[x9]
    ldr w1,[x9,#0x8]
*/
    bl kernel_start
    //;; hack to avoid stack protector
    //;; mov DWORD [0x1014], 0x89abcdef

fast_kernel_start:
    /*;; Push params on 16-byte aligned stack
    ;; sub esp, 8
    ;; and esp, -16
    ;; mov [esp], eax
    ;; mov [esp+4], ebx
*/
//    b kernel_start

  /*  ;; Restore stack frame
    ;; mov esp, ebp
    ;; pop ebp
    ;; ret
*/
