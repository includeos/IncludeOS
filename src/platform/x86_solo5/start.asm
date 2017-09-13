;; This file is a part of the IncludeOS unikernel - www.includeos.org
;;
;; Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
;; and Alfred Bratterud
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;;     http:;;www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;; See the License for the specific language governing permissions and
;; limitations under the License.

global __multiboot_magic
global __multiboot_addr
global set_stack

%define  MB_MAGIC   0x1BADB002
%define  MB_FLAGS   0x3  ;; ALIGN + MEMINFO

extern _MULTIBOOT_START_
extern _LOAD_START_
extern _LOAD_END_
extern _end
extern _start
extern kernel_start

ALIGN 4
section .multiboot
  dd  MB_MAGIC
  dd  MB_FLAGS
  dd  -(MB_MAGIC + MB_FLAGS)
  dd _MULTIBOOT_START_
  dd _LOAD_START_
  dd _LOAD_END_
  dd _end
  dd _start

section .data
__multiboot_magic:
    dd 0x0
__multiboot_addr:
    dd 0x0

set_stack:
  mov esp, 0xA0000
  mov ebp, esp
  call kernel_start
