
ENTRY(_start)

SECTIONS
{
  .text (0x200000+SIZEOF_HEADERS) :
  {
    _TEXT_START_ = .;
    *(.text)
    *(.text.*)
    *(.gnu.linkonce.t*)
    _TEXT_END_ = .;
  }
  
  .init ALIGN(0x10) : {
    _INIT_START_ = .;
    *(.init)
    _INIT_END_ = .; 
  }
  .fini ALIGN(0x10) : {
    /* we don't want destructors taking up space */
    *(.fini)
  }
  
  /* Global constructor array */
  .ctors ALIGN(0x10) :
  {
    _GCONSTR_START = .;
    *(.ctors)
    _GCONSTR_END = .;
    LONG(0);
  }
  .dtors ALIGN(0x10) :
  {
    *(.dtors)
    LONG(0);
  }
  
  .rodata :
  {
    _RODATA_START_ = .;
    *(.rodata*)
    *(.gnu.linkonce.r*)
    _RODATA_END_ = .;
  }
  
  /* For stack unwinding (Required by exception handling)  */
  eh_frame ALIGN(0x4):
  {
    PROVIDE (__eh_frame_start = .);
    KEEP(*(.eh_frame))
    LONG (0);
  }
  .gcc_except_table :
  {
    *(.gcc_except_table)
  }
  
  .data :
  {
    _DATA_START_ = .;
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d*)
    _DATA_END_ = .;
  }
  
  .syms :
  {
    *(.symtab)
    *(.strtab)
  }
  
  .memdisk :
  {
    _DISK_START_ = .;
    *(.diskdata)
    _DISK_END_ = .;
  }
  
  .bss :
  {
    _BSS_START_ = .;
    *(.bss)
    *(.bss.*)
    *(.gnu.linkonce.b.*)
    _BSS_END_ = .;
  }
  
  _includeos = .;
  . = ALIGN(4096);
  _end = .;
}
