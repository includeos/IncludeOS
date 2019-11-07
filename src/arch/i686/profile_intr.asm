USE32
extern current_intr_handler

global parasite_interrupt_handler:function
extern profiler_stack_sampler

parasite_interrupt_handler:
  cli
  pusha
  push DWORD [esp + 32]
  call profiler_stack_sampler
  pop eax
  call DWORD [current_intr_handler]
  popa
  sti
  iret
