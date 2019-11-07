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
