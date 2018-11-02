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

global __arch_start:function
global fast_kernel_start:function
extern kernel_start

[BITS 32]

;; @param: eax - multiboot magic
;; @param: ebx - multiboot bootinfo addr
__arch_start:

    ;; Create stack frame for backtrace
    push ebp
    mov ebp, esp

    ;; hack to avoid stack protector
    mov DWORD [0x1014], 0x89abcdef

fast_kernel_start:
    ;; Push params on 16-byte aligned stack
    sub esp, 8
    and esp, -16
    mov [esp], eax
    mov [esp+4], ebx

    call kernel_start

    ;; Restore stack frame
    mov esp, ebp
    pop ebp
    ret
