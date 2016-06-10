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
    *(.fini)
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
  _EH_FRAME_START_ = .;
  
  .eh_frame ALIGN(0x4):
  {
    PROVIDE (__eh_frame_start = .);
    KEEP(*(.eh_frame))
    LONG (0);
  }
  
  _EH_FRAME_END_ = .;
  
  .data :
  {
    _DATA_START_ = .;
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d*)
    _DATA_END_ = .;
  }
  
  .symbols ALIGN(0x4) :
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
  
  .pagealign ALIGN(4096): {
    _includeos = .;
  }
  _end = .;
}
