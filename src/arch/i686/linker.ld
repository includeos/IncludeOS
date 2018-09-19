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
  PROVIDE ( _ELF_START_ = . + 0x100000);
  PROVIDE ( _LOAD_START_ = _ELF_START_); /* For convenience w. multiboot */

  . = _ELF_START_ + SIZEOF_HEADERS;

  .multiboot : {
      PROVIDE(_MULTIBOOT_START_ = .);
      *(.multiboot)
   }

  .text ALIGN(0x1000):
  {
  PROVIDE( _TEXT_START_ = . );
    *(.text)
    *(.text.*)
    *(.gnu.linkonce.t*)
  }
  PROVIDE( _TEXT_END_ = . );

  /* Global offset-table. For dynamic linking */
  .got ALIGN(0x10) : {
      *(.got*)
  }

/**
 *  .preinit_array, .init_array, .fini_array
 *  from GNU LD default linker script
 */

  .preinit_array :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  }

/**
  *   Global constructors
  *   Constructors are split into groups allowing the OS to use global ctors
  *   before the OS itself is initialized, while delaying the calls to service constructors
  *   until as much of the OS / C++ runtime as possible is ready.
  */

  /* OS / stdlib constructors */
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    */x86_64/lib/lib*.a:*(.init_array* .ctors*)
    */x86_64/platform/lib*.a:*(.init_array* .ctors*)
    */x86_64/drivers/lib*.a:*(.init_array* .ctors*)
    PROVIDE_HIDDEN (__init_array_end = .);
  }

  /* Stdout g constructors */
  .stdout_ctors :
  {
    PROVIDE_HIDDEN (__stdout_ctors_start = .);
    */x86_64/drivers/stdout/lib*.a:*(.init_array* .ctors*)
    PROVIDE_HIDDEN (__stdout_ctors_end = .);
  }

  /* Driver g constructors */
  .driver_ctors :
  {
    PROVIDE_HIDDEN (__driver_ctors_start = .);
    */x86_64/drivers/lib*.a:*(.init_array* .ctors*)
    PROVIDE_HIDDEN (__driver_ctors_end = .);
  }

  /* Plugin constructors */
  .plugin_ctors :
  {
    PROVIDE_HIDDEN (__plugin_ctors_start = .);
    */x86_64/plugins/lib*.a:*(.init_array* .ctors*)
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
