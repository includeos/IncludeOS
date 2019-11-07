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
