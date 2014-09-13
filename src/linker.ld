
ENTRY(_start)

SECTIONS
{
  .text.start (0x8000+SIZEOF_HEADERS) : {
    kernel_start.o( .text )
  }

  .text : {
   _TEXT_START_ = .;
    *(.text)
   _TEXT_END_ = .;
  }

  .init : {	
  	_INIT_START_ = .;
	*(.init)
	_INIT_END_ = .;	
  }  	
  
  .ctors : {
  	 *(.ctors)
  }
  .dtors : {
  	 *(.dtors)
  }
   .rodata :  {
_RODATA_START_ = .;
    *(.rodata)
_RODATA_END_ = .;
  }

  
  .data :  {
_DATA_START_ = .;
    *(.data)
_DATA_END_ = .;
  }

  
   .bss :  { _BSS_START_ = .;  *(.bss) _BSS_END_ = .;  }
   _includeos = .;
  _end = .;  
}



