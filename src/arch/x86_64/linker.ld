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

SECTIONS
{

  PROVIDE (__executable_start = SEGMENT_START("text-segment", 0x800000));

  /* For convenience w. multiboot */
  PROVIDE ( _ELF_START_ = __executable_start);
  PROVIDE ( _LOAD_START_ = _ELF_START_ );

  . = SEGMENT_START("text-segment", 0x800000) + SIZEOF_HEADERS;

  .multiboot : {
      PROVIDE(_MULTIBOOT_START_ = . );
      *(.multiboot)
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
  .init ALIGN(0x10) : {
    _INIT_START_ = .;
    *(.init)
    _INIT_END_ = .;
  }

  .fini ALIGN(0x10) : {
    _FINI_START_ = .;
    *(.fini)
    _FINI_END_ = .;
  }

  /* Global offset-table. For dynamic linking */
  .got ALIGN(0x10) : {
      *(.got*)
  }


  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
    KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
    PROVIDE_HIDDEN (__init_array_end = .);
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

  .eh_frame :
  {
    PROVIDE (__eh_frame_start = .);
    KEEP(*(.eh_frame))
    PROVIDE (__eh_frame_end = .);
  }

  .eh_frame_hdr :
  {
    KEEP(*(.eh_frame_hdr))
  }

  .gcc_except_table :
  {
    *(.gcc_except_table)
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

  .tdata ALIGN(0x10) :
  {
    _TDATA_START_ = .;
    *(.tdata .tdata.*)
    _TDATA_END_ = .;
    . = ALIGN(0x10);
  }

  .tbss :
  {
    _TBSS_START_ = .;
    *(.tbss .tbss.*)
    _TBSS_END_ = .;
    . = ALIGN(0x10);
  }

  .memdisk :
  {
    _DISK_START_ = .;
    *(.diskdata)
    _DISK_END_ = .;
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
