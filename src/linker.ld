
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
  
  .init :
  {
    _INIT_START_ = .;
    *(.init)
    _INIT_END_ = .; 
  }
  
  /* Global constructor array */
  .ctors :
  {
    _GCONSTR_START = .;
    *(.ctors)
    _GCONSTR_END = .;
    LONG(0);
  }
  .dtors :
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
    
    /* this will make the image bigger, but maybe give it some speed too */
    /*. = ALIGN(4096);*/
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
  _end = .;  
}
