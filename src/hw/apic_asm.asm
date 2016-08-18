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
global apic_enable
global spurious_intr
global lapic_send_eoi
global get_cpu_id
global get_cpu_eip
global get_cpu_esp
global reboot

global lapic_irq_entry

apic_enable:
    push ecx
    push eax
    mov			ecx, 1bh
    rdmsr
    bts			eax, 11
    wrmsr
    pop eax
    pop ecx
    ret

get_cpu_id:
    mov eax, 1
    cpuid
    shr ebx, 24
    mov eax, ebx
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
    push eax
    mov eax, 0xfee000B0
    mov DWORD [eax], 0
    pop eax
    ret

reboot:
    ; load bogus IDT
    lidt [reset_idtr]
    ; 1-byte breakpoint instruction
    int3

reset_idtr:
    dw      400h - 1
    dd      0

lapic_irq_entry:
    call lapic_send_eoi
    iret
