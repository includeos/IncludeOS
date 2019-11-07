ENTRY(_start)

SECTIONS
{
  PROVIDE ( _ELF_START_ = . + 0x200000);
  PROVIDE ( _LOAD_START_ = _ELF_START_); /* For convenience w. multiboot */

  . = _ELF_START_ + SIZEOF_HEADERS;

  .multiboot : {
      PROVIDE(_MULTIBOOT_START_ = .);
      KEEP(*(.multiboot))
   }

  .text ALIGN(0x1000):
  {
  PROVIDE( _TEXT_START_ = . );
    /* For solo5, although it's just used to print the mem layout. */
    _stext = .;
    *(.text)
    *(.text.*)
    *(.gnu.linkonce.t*)
    /* For solo5, although it's just used to print the mem layout. */
    _etext = .;
  }
  PROVIDE( _TEXT_END_ = . );

/**
  *   Global constructors
  *   Constructors are split into groups allowing the OS to use global ctors
  *   before the OS itself is initialized, while delaying the calls to service constructors
  *   until as much of the OS / C++ runtime as possible is ready.
  */

  .preinit_array :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  }

  /* OS / stdlib constructors */
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    */lib/lib*.a:*(.init_array* .ctors*)
    */platform/lib*.a:*(.init_array* .ctors*)
    PROVIDE_HIDDEN (__init_array_end = .);
  }

  /* Stdout g constructors */
  .stdout_ctors :
  {
    PROVIDE_HIDDEN (__stdout_ctors_start = .);
    */drivers/stdout/lib*.a:*(.init_array* .ctors*)
    PROVIDE_HIDDEN (__stdout_ctors_end = .);
  }

  /* Driver g constructors */
  .driver_ctors :
  {
    PROVIDE_HIDDEN (__driver_ctors_start = .);
    */drivers/lib*.a:*(.init_array* .ctors*)
    PROVIDE_HIDDEN (__driver_ctors_end = .);
  }

  /* Plugin g constructors */
  .plugin_ctors :
  {
    PROVIDE_HIDDEN (__plugin_ctors_start = .);
    */plugins/lib*.a:*(.init_array* .ctors*)
    PROVIDE_HIDDEN (__plugin_ctors_end = .);
  }

  /* All other constructors */
  .service_ctors :
  {
    PROVIDE_HIDDEN (__service_ctors_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
    KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
    PROVIDE_HIDDEN (__service_ctors_end = .);
  }

  .fini_array :
  {
   PROVIDE_HIDDEN (__fini_array_start = .);
   KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
   KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
   PROVIDE_HIDDEN (__fini_array_end = .);
  }

  _EXEC_END_ = .;
  _READONLY_START_ = .;
  .config ALIGN(0x1000) : {
    _CONFIG_JSON_START_ = .;
    KEEP(*(.config))
    _CONFIG_JSON_END_ = .;
    BYTE(0);
  }

  .rodata :
  {
    _RODATA_START_ = .;
    *(.rodata*)
    *(.gnu.linkonce.r*)
    _RODATA_END_ = .;
    /* For solo5, although it's just used to print the mem layout. */
    _erodata = .;
  }

  .memdisk :
  {
    _DISK_START_ = .;
    KEEP(*(.diskdata))
    _DISK_END_ = .;
  }

  /* For stack unwinding (exception handling)  */
  .eh_frame_hdr ALIGN(0x8):
  {
    KEEP(*(.eh_frame_hdr*))
  }
  .eh_frame ALIGN(0x8):
  {
    PROVIDE (__eh_frame_start = .);
    KEEP(*(.eh_frame))
    LONG (0);
  }
  .gcc_except_table : {
    *(.gcc_except_table*)
  }

  _READONLY_END_ = .;

  .data :
  {
    _DATA_START_ = .;
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d*)
    _DATA_END_ = .;
  }

  .tdata :
  {
    _TDATA_START_ = .;
    *(.tdata .tdata.* .gnu.linkonce.td.*)
    _TDATA_END_ = .;
  }

  .tbss :
  {
    _TBSS_START_ = .;
    *(.tbss .tbss.* .gnu.linkonce.tb.*) *(.tcommon)
    _TBSS_END_ = .;
  }

  .elf_symbols : {
    _ELF_SYM_START_ = .;
    LONG (0);
  }

  /** Optional memory hole between memdisk and bss **/
  . += PRE_BSS_AREA;

  .bss ALIGN(0x1000) :
  {
    _BSS_START_ = .;
    *(.bss .bss.* .gnu.linkonce.b.*)
    *(COMMON)
    _BSS_END_ = .;
  }

  . = ALIGN(0x8);
  _end = .;

  PROVIDE (end = .);
  PROVIDE (_ELF_END_ = .);
  PROVIDE (_LOAD_END_ = .);
}
