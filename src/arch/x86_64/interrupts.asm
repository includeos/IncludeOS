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
[BITS 64]
global unused_interrupt_handler:function
global modern_interrupt_handler:function
global cpu_sampling_irq_entry:function
global blocking_cycle_irq_entry:function
global parasite_interrupt_handler:function

extern current_eoi_mechanism
extern current_intr_handler
extern cpu_sampling_irq_handler
extern blocking_cycle_irq_handler
extern profiler_stack_sampler

SECTION .bss
ALIGN 16
__xsave_storage_area: resb  512

%macro PUSHAQ 0
   push rax
   push rcx
   push rdx
   push rdi
   push rsi
   push r8
   push r9
   push r10
   push r11

   ; Preserve extended state
   ;fxsave [__xsave_storage_area]
%endmacro
%macro POPAQ 0
   ; Restore extended state
   ;fxrstor [__xsave_storage_area]

   pop r11
   pop r10
   pop r9
   pop r8
   pop rsi
   pop rdi
   pop rdx
   pop rcx
   pop rax
%endmacro


SECTION .text
unused_interrupt_handler:
  cli
  PUSHAQ
  call QWORD [current_eoi_mechanism]
  POPAQ
  sti
  iretq

modern_interrupt_handler:
  cli
  PUSHAQ
  call QWORD [current_intr_handler]
  POPAQ
  sti
  iretq

cpu_sampling_irq_entry:
  cli
  PUSHAQ
  call cpu_sampling_irq_handler
  call QWORD [current_eoi_mechanism]
  POPAQ
  sti
  iretq

blocking_cycle_irq_entry:
  cli
  PUSHAQ
  call blocking_cycle_irq_handler
  call QWORD [current_eoi_mechanism]
  POPAQ
  sti
  iretq

parasite_interrupt_handler:
  cli
  PUSHAQ
  mov  rdi, QWORD [rsp + 8*9]
  call profiler_stack_sampler
  call QWORD [current_intr_handler]
  POPAQ
  sti
  iretq
