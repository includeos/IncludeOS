ENTRY(_start)

SECTIONS {
    . = 0x100000;
    PROVIDE ( _ELF_START_ = .);
    PROVIDE ( _LOAD_START_ = _ELF_START_);

    .multiboot (_ELF_START_ + SIZEOF_HEADERS): {
      PROVIDE(_MULTIBOOT_START_ = .);
      *(.multiboot)
    }

    /* Code */
    _stext = .;

    PROVIDE( _TEXT_START_ = . );
    .text (_TEXT_START_ ) :
    {
        *(.text)
        *(.text.*)
    }
    PROVIDE( _TEXT_END_ = . );

    . = ALIGN(0x1000);
    _etext = .;

    /* Read-only data */
    .rodata :
    {
        _RODATA_START_ = .;
        *(.rodata)
        *(.rodata.*)
        _RODATA_END_ = .;
    }


    /* For stack unwinding (exception handling)  */
    .eh_frame_hdr ALIGN(0x8):
    {
      KEEP(*(.eh_frame_hdr*))
    }

    .eh_frame :
    {
        PROVIDE (__eh_frame_start = .);
        *(.eh_frame)
    }

    . = ALIGN(0x1000);
    _erodata = .;

    /* Read-write data (initialized) */
    .got :
    {
        *(.got.plt)
        *(.got)
    }
    .data :
    {
        *(.data)
        *(.data.*)
    }

    _edata = .;

    . = ALIGN(0x1000);

    .elf_symbols : {
      _ELF_SYM_START_ = .;
      LONG (0);
    }

    /* XXX: We shouldn't have to do this. */
    . = ALIGN(0x100000);

    /* Read-write data (uninitialized) */
    .bss :
    {
        _BSS_START_ = .;
        *(.bss .bss.* .gnu.linkonce.b.*)
        *(COMMON)
        _BSS_END_ = .;
    }

    . = ALIGN(0x1000);
    _ebss = .;
    _end = .;

    PROVIDE (_ELF_END_ = .);
    PROVIDE (_LOAD_END_ = .);
}
