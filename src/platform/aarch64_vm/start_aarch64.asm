//.globl reg_x1
//.globl cafe
//.globl reg_x0
.globl __boot_magic

.data
.align 8
__boot_magic:
  .dword 0

.text
.align 8
.globl _start
_start:
  b reset

.globl reset
reset:
  //mov x0, #0xcafe
  ldr x8 , __boot_magic
  str x0, [x8]

/*  ldr x8, reg_x1
  str x1, [x8]
*/

#if defined(SET_EXCEPTION_HANDLERS)
.macro  set_vbar, regname, reg
        msr     \regname, \reg
.endm
        adr     x0, vectors
#else
.macro  set_vbar, regname, reg
.endm
#endif
        /*
         * Could be EL3/EL2/EL1, Initial State:
         * Little Endian, MMU Disabled, i/dCache Disabled
         */
        //switch_el x1, 3f, 2f, 1f u boot uses a macro do switch based on exception  level..
          /*
           * Branch according to exception level
           */
        /*
        *  .macro  switch_el, xreg, el3_label, el2_label, el1_label
        *          mrs     \xreg, CurrentEL
        *          cmp     \xreg, 0xc
        *          b.eq    \el3_label
        *          cmp     \xreg, 0x8
        *          b.eq    \el2_label
        *          cmp     \xreg, 0x4
        *          b.eq    \el1_label
        *  .endm
        */
        //do we need this switch?
        mrs     x1, CurrentEL //load current execution level
        cmp     x1, 0xc
        b.eq    3f
        cmp     x1, 0x8
        b.eq    2f
        cmp     x1, 0x4
        b.eq    1f
3:      set_vbar vbar_el3, x0
        mrs     x0, scr_el3
        orr     x0, x0, #0xf                    /* SCR_EL3.NS|IRQ|FIQ|EA */
        msr     scr_el3, x0
        msr     cptr_el3, xzr                   /* Enable FP/SIMD */
#ifdef COUNTER_FREQUENCY
        ldr     x0, =COUNTER_FREQUENCY
        msr     cntfrq_el0, x0                  /* Initialize CNTFRQ */
#endif
        b       0f
2:      set_vbar        vbar_el2, x0
        mov     x0, #0x33ff
        msr     cptr_el2, x0                    /* Enable FP/SIMD */
        b       0f
1:      set_vbar        vbar_el1, x0
        mov     x0, #3 << 20
        msr     cpacr_el1, x0                   /* Enable FP/SIMD */
0:


  b __arch_start
