#include "class_irq_handler.h"

IDTDescr idt[256];

void IRQ_handler::set_IDT(){
  /*  for(int i=0;i<256;i++){
    idt[i].selector=0x10;
    idt[
    }*/
  idt_loc idt_reg;
  idt_reg.limit=(256*sizeof(IDTDescr))-1;
    
  printf("\n** IRQ handler setting idt \n");
  create_gate(&(idt[0]),handle_IRQ_default,0x10,'a','b');
};

void IRQ_handler::create_gate(IDTDescr* idt_entry,
			      void (*function_addr)(),
			      uint16_t segment_sel,
			      char type,
			      char privilege)
{
  printf("\t>>>  Creating IRQ gate...\n");
  function_addr();
}

void IRQ_handler::handle_IRQ_default(){
  printf("\t>>>  Handling IRQ...\n");
}
