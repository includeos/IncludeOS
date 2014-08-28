
ENTRY(_start)

SECTIONS
{
  .text.start (0x8080) : {
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
	*(.ctors)
  }  	


   .rodata :  {
_RODATAZ_START_ = .;
    *(.rodata)
_RODATAZ_END_ = .;
  }

  _end = .;
  .data :  {
_DATA_START_ = .;
    *(.data)
_DATA_END_ = .;
  }


  .bss :  { _BSS_START_ = .;  *(.bss) _BSS_END_ = .;  }
  
}

_includeos = 0xf0000;


