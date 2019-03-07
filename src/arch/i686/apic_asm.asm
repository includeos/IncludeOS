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
USE32
global spurious_intr:function
global lapic_send_eoi:function
global get_cpu_id:function
global get_cpu_eip:function
global get_cpu_esp:function
global reboot_os:function

section .text
get_cpu_id:
    mov eax, [fs:0x0]
    ret

get_cpu_esp:
    mov eax, esp
    ret

get_cpu_eip:
    mov eax, [esp]
    ret

spurious_intr:
    iret

lapic_send_eoi:
    mov eax, 0xfee000B0
    mov DWORD [eax], 0
    ret

reboot_os:
    ; load bogus IDT
    lidt [reset_idtr]
    ; 1-byte breakpoint instruction
    int3

reset_idtr:
    dw      400h - 1
    dd      0
