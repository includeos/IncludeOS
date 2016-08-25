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

USE32
section .text

extern kernel_start
extern _LOAD_START_
extern __stack_rand_ba

global _start

;; Multiboot places boot paramters on eax and ebx.
_start:
        ;; Stack base address
        mov esp, _LOAD_START_
        ;; Primitive stack base address randomization
        mov ecx, __stack_rand_ba
        and ecx, 0xff
        shl ecx, 10 ;; up to 256kb per 256 seconds
        sub esp, ecx
        mov [boot_magic], eax
        rdtsc
        and eax, 0xff
        shl eax, 6 ;; 64 byte per tick, up to 16kb

        ;; NOTE: Stack changes here (pushes before this point won't pop right)
        sub esp, eax

        ;; make esp page-aligned
        and esp, 0xfffff000

        ;;  Place multiboot parameters on stack
        mov eax, [boot_magic]
        push ebx
        push eax
        call kernel_start

boot_magic:
        dw 0
