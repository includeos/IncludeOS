
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
  
  .ctors :
  {
    *(.ctors)
  }
  .dtors :
  {
    *(.dtors)
  }
  
  
  /* For stack unwinding (Required by exception handling)  */
   .eh_frame ALIGN(0x4):
    { 
      PROVIDE (__eh_frame_start = .);
      KEEP(*(.eh_frame))
      LONG (0);
    } 

   .rodata :  {
_RODATA_START_ = .;
    *(.rodata)
_RODATA_END_ = .;
  }
  
  
  .data :
  {
    _DATA_START_ = .;
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d*)
    _DATA_END_ = .;
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
