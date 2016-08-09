/**
 * This file is a part of the IncludeOS unikernel - www.includeos.org
 *
 * Copyright 2015 Oslo and Akershus University College of Applied Sciences
 * and Alfred Bratterud
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http: *www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
**/
ENTRY(_start)

PHDRS {
    loadable PT_LOAD FLAGS(7) ;
}

SECTIONS
{
  PROVIDE ( _ELF_START_ = . + 0x200000);
  PROVIDE ( _LOAD_START_ = _ELF_START_); /* For convenience w. multiboot */

  .multiboot (_ELF_START_ + SIZEOF_HEADERS): {
      PROVIDE(_MULTIBOOT_START_ = .);
      *multiboot.o(.rodata);
   }
  PROVIDE ( _TEXT_START_ = . );
  .text (_TEXT_START_ ) :
  {
    *(.text)
    *(.text.*)
    *(.gnu.linkonce.t*)
  }

  .init ALIGN(0x10) : {
    _INIT_START_ = .;
    *(.init)
    _INIT_END_ = .;
  }

  .fini ALIGN(0x10) : {
    *(.fini)
  }

  /* Global offset-table. For dynamic linking */
  .got ALIGN(0x10) : {
      *(.got*)
  }
  /* Global constructor array */
  .ctors ALIGN(0x10) :
  {
    _GCONSTR_START_ = .;
    *(.ctors)
    _GCONSTR_END_ = .;
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

  .gcc_except_table :
  {
    *(.gcc_except_table)
  }

    /* For stack unwinding (exception handling)  */
  .eh_frame_hdr ALIGN(0x4):
  {
    KEEP(*(.eh_frame_hdr*))

  }
  .eh_frame ALIGN(0x4):
  {
    PROVIDE (__eh_frame_start = .);
    KEEP(*(.eh_frame))
    LONG (0);
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

  .symtab :
  {
        _SYMTAB_START = .;
        *(.symtab)
        *(.symtab.*)
  }:loadable

  .strtab :
  {
        _STRTAB_START = .;
        *(.strtab)
        *(.strtab.*)
  }:loadable

  .bss :
  {
    _BSS_START_ = .;
    *(.bss .bss.* .gnu.linkonce.b.*)
    *(COMMON)
    _BSS_END_ = .;
    . = ALIGN(64 / 8);
  }
  . = ALIGN(64 / 8);
  _end = .;
  PROVIDE (end = .);
  PROVIDE (_ELF_END_ = .);
  PROVIDE (_LOAD_END_ = .);


}
