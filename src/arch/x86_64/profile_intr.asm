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
extern current_intr_handler

global parasite_interrupt_handler:function
extern profiler_stack_sampler

%macro PUSHAQ 0
   push rax
   push rbx
   push rcx
   push rdx
   push rbp
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
%endmacro
%macro POPAQ 0
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
   pop rbp
   pop rdx
   pop rcx
   pop rbx
   pop rax
%endmacro

parasite_interrupt_handler:
  cli
  PUSHAQ
  mov  rax, rsp
  push rsp
  mov  rsi, QWORD [rsp + 32]
  call profiler_stack_sampler
  call QWORD [current_intr_handler]
  POPAQ
  sti
  iretq
