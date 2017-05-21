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
global parasite_interrupt_handler:function
global __stack_storage_area

extern current_eoi_mechanism
extern current_intr_handler
extern cpu_sampling_irq_handler
extern profiler_stack_sampler

SECTION .bss
ALIGN 16
__xsave_storage_area: resb  512
ALIGN 16
__stack_storage_area: resb  512

%macro PUSHAQ 0
   push rax
   push rbx
   push rcx
   push rdx
   push rdi
   push rsi
   push r8
   push r9
   push r10
   push r11
   push r12
   push r13
   push r14
   push r15

   ; Preserve extended state
   ;fxsave [__xsave_storage_area]
%endmacro
%macro POPAQ 0
   ; Restore extended state
   ;fxrstor [__xsave_storage_area]

   pop r15
   pop r14
   pop r13
   pop r12
   pop r11
   pop r10
   pop r9
   pop r8
   pop rsi
   pop rdi
   pop rdx
   pop rcx
   pop rbx
   pop rax
%endmacro

%macro SPSAVE 0
    mov  rax, rsp
    mov  rsp, __stack_storage_area+512-16
    push rax ;; save old RSP
    push rbp
%endmacro
%macro SPRSTOR 0
    pop rbp
    pop rsp  ;; restore old RSP
%endmacro


SECTION .text
unused_interrupt_handler:
  cli
  PUSHAQ
  SPSAVE
  call QWORD [current_eoi_mechanism]
  SPRSTOR
  POPAQ
  sti
  iretq

modern_interrupt_handler:
  cli
  PUSHAQ
  SPSAVE
  call QWORD [current_intr_handler]
  SPRSTOR
  POPAQ
  sti
  iretq

cpu_sampling_irq_entry:
  cli
  PUSHAQ
  SPSAVE
  call cpu_sampling_irq_handler
  call QWORD [current_eoi_mechanism]
  SPRSTOR
  POPAQ
  sti
  iretq

parasite_interrupt_handler:
  cli
  PUSHAQ
  mov  rdi, QWORD [rsp + 8*14]
  SPSAVE
  call profiler_stack_sampler
  call QWORD [current_intr_handler]
  SPRSTOR
  POPAQ
  sti
  iretq
