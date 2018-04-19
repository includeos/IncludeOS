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
OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

ENTRY(_start)

SECTIONS
{

  PROVIDE (__executable_start = SEGMENT_START("text-segment", 0xa00000));
  . = SEGMENT_START("text-segment", 0xa00000) + SIZEOF_HEADERS;
  . = _ELF_START_ + SIZEOF_HEADERS;

  /* For convenience w. multiboot */
  PROVIDE ( _ELF_START_ = __executable_start);
  PROVIDE ( _LOAD_START_ = _ELF_START_ );

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
  . = SEGMENT_START("data-segment", .);
  . = DATA_SEGMENT_ALIGN (CONSTANT (MAXPAGESIZE), CONSTANT (COMMONPAGESIZE));
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

/**
 *  .ctors, .dtors, .preinit_array, .init_array, .fini_array
 *  from GNU LD default linker script
 */

.ctors          :
  {
    _GCONSTR_START_ = .;
      /* gcc uses crtbegin.o to find the start of
       the constructors, so we make sure it is
       first.  Because this is a wildcard, it
       doesn't matter if the user does not
       actually link against crtbegin.o; the
       linker won't look for a file to match a
       wildcard.  The wildcard also means that it
       doesn't matter which directory crtbegin.o
       is in.  */
       KEEP (*crtbegin.o(.ctors))
       KEEP (*crtbegin?.o(.ctors))
       /* We don't want to include the .ctor section from
       the crtend.o file until after the sorted ctors.
       The .ctor section from the crtend file contains the
       end of ctors marker and it must be last */
       KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
       KEEP (*(SORT(.ctors.*)))
       KEEP (*(.ctors))
    _GCONSTR_END_ = .;
  }

  .dtors :
  {
    KEEP (*crtbegin.o(.dtors))
    KEEP (*crtbegin?.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
  }

  .preinit_array :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);
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

  .rodata ALIGN(0x1000):
  {
    _RODATA_START_ = .;
    *(.rodata*)
    *(.gnu.linkonce.r*)
    _RODATA_END_ = .;
    /* For solo5, although it's just used to print the mem layout. */
    _erodata = .;
  }

  .config ALIGN(0x8) : {
    _CONFIG_JSON_START_ = .;
    KEEP(*(.config))
    _CONFIG_JSON_END_ = .;
    BYTE(0);
  }


  /* For stack unwinding (exception handling)
     Taken from https://github.com/llvm-mirror/libunwind/blob/master/src/AddressSpace.hpp
     Linker script reference in the comments*/

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
  . = DATA_SEGMENT_END (.);

  PROVIDE (end = .);
  PROVIDE (_ELF_END_ = .);
  PROVIDE (_LOAD_END_ = .);
}
