/*
 * Branch according to exception level
 */
.macro  execution_level_switch, var, el3, el2, el1
        mrs     \var, CurrentEL
        cmp     \var, 0xc
        b.eq    \el3
        cmp     \var, 0x8
        b.eq    \el2
        cmp     \var, 0x4
        b.eq    \el1
.endm
