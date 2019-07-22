USE32
global unused_interrupt_handler:function
global modern_interrupt_handler:function
global cpu_sampling_irq_entry:function
global blocking_cycle_irq_entry:function

extern current_eoi_mechanism
extern current_intr_handler
extern cpu_sampling_irq_handler
extern blocking_cycle_irq_handler

unused_interrupt_handler:
  cli
  pusha
  call DWORD [current_eoi_mechanism]
  popa
  sti
  iret

modern_interrupt_handler:
  cli
  pusha
  call DWORD [current_intr_handler]
  popa
  sti
  iret

cpu_sampling_irq_entry:
  cli
  pusha
  call cpu_sampling_irq_handler
  call DWORD [current_eoi_mechanism]
  popa
  sti
  iret

blocking_cycle_irq_entry:
  cli
  pusha
  call blocking_cycle_irq_handler
  call DWORD [current_eoi_mechanism]
  popa
  sti
  iret
