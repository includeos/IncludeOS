/*
; This file is a part of the IncludeOS unikernel - www.includeos.org
;
; Copyright 2015 Oslo and Akershus University College of Applied Sciences
; and Alfred Bratterud
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.
*/
.text
.global __arch_start
.global fast_kernel_start
.global init_serial

//linker handles this
//.extern kernel_start
//not sure if this is a sane stack location
.set STACK_LOCATION, 0x200000 - 16
.extern __stack_top
//__stack_top:

//;; @param: eax - multiboot magic
//;; @param: ebx - multiboot bootinfo addr
.align 8
__arch_start:
    //store any boot magic if present ?
    ldr x8,__boot_magic
    ldr x0,[x8]
    //
    mrs x0, s3_1_c15_c3_0
  //  ;; Create stack frame for backtrace

    ldr x30, =__stack_top
    mov sp, x30


  /*  ldr w0,[x9]
    ldr w1,[x9,#0x8]
*/
    bl kernel_start
    //;; hack to avoid stack protector
    //;; mov DWORD [0x1014], 0x89abcdef

fast_kernel_start:
    /*;; Push params on 16-byte aligned stack
    ;; sub esp, 8
    ;; and esp, -16
    ;; mov [esp], eax
    ;; mov [esp+4], ebx
*/
//    b kernel_start

  /*  ;; Restore stack frame
    ;; mov esp, ebp
    ;; pop ebp
    ;; ret
*/
