#include "class_irq_handler.h"

bool IRQ_handler::idt_is_set=false;
IDTDescr IRQ_handler::idt[256];

void IRQ_handler::enable_interrupts(){
  __asm__ volatile("sti");
}

extern "C"{
  void default_irq_handler(int a, int b, int c);
  void default_irq_entry();

  void timer_irq_entry();
  void timer_irq_handler();

  /*
    Macro for a default IRQ-handler, which just prints its number
   */
#define DEFAULT_HANDLER(I) \
  void irq_##I##_handler(){ \
    printf("IRQ %i \n",I);  \
  }

  /*
    Define two functions for each IRQ i :
    > void irq_i_entry() - defined in interrupts.s
    > void irq_i_handler() - defined here.
   */

#define IRQ_PAIR(I) void irq_##I##_entry(); DEFAULT_HANDLER(I);
  IRQ_PAIR(0);
  IRQ_PAIR(1);
  IRQ_PAIR(2);
  IRQ_PAIR(3);
  IRQ_PAIR(4);
  IRQ_PAIR(5);
  IRQ_PAIR(6);
  IRQ_PAIR(7);
  IRQ_PAIR(8);
  IRQ_PAIR(9);
  IRQ_PAIR(10);
  IRQ_PAIR(11);
  IRQ_PAIR(12);
  IRQ_PAIR(13);
  IRQ_PAIR(14);
  IRQ_PAIR(15);
} //End extern

inline void disable_pic();

enum{
  CPUID_FEAT_EDX_APIC = 1 << 9
};

static inline void cpuid(int code, uint32_t *a, uint32_t *d) {
  asm volatile("cpuid"
	       :"=a"(*a),"=d"(*d)
	       :"a"(code)
	       :"ecx","ebx");
}

bool cpuHasAPIC()
{
  uint32_t eax, edx;
  cpuid(1, &eax, &edx);
  return edx & CPUID_FEAT_EDX_APIC;
}

void IRQ_handler::set_IDT(){
  printf("CPU HAS APIC: %s \n", cpuHasAPIC() ? "YES" : "NO" );
  if(idt_is_set){
    printf("ERROR: Trying to reset IDT");
    kill(1,9);
  }
  //Create an idt entry for the 'lidt' instruction
  idt_loc idt_reg;
  idt_reg.limit=(256*sizeof(IDTDescr))-1;
  idt_reg.base=(uint32_t)idt;
  
  printf("\n** IRQ handler setting idt \n");
  //disable_pic();
  

/*
//STEAL OS-course
#define IRQ_START 32

// interrupt controller 1 
OS::outb(0x20, 0x11); // Start init of controller 0, require 4 bytes
OS::outb(0x21, IRQ_START);  // IRQ 0-7 use vectors 0x20-0x27 (32-39)
OS::outb(0x21, 0x04);  // Slave controller on IRQ 2
OS::outb(0x21, 0x01);  // Normal EOI nonbuffered, 80x86 mode
OS::outb(0x21, 0xfb);  // Disable int 0-7, enable int 2
// interrupt controller 2
OS::outb(0xa0, 0x11);  //Start init of controller 1, require 4 bytes
OS::outb(0xa1, IRQ_START + 8); // IRQ 8-15 use vectors 0x28-0x30 (40-48)
OS::outb(0xa1, 0x02);  // Slave controller id, slave on IRQ 2
OS::outb(0xa1, 0x01);  // Normal EOI nonbuffered, 80x86 mode
OS::outb(0xa1, 0xff);  // Disable int 8-15
*/
  

  
  //Macro magic to create default gates
#define DEFAULT_GATE(I) create_gate(&(idt[I]),irq_##I##_entry,\
				    default_sel, default_attr );
  //Assign the lower 16 IRQ's
  DEFAULT_GATE(0);
  DEFAULT_GATE(1);
  DEFAULT_GATE(2);
  DEFAULT_GATE(3);
  DEFAULT_GATE(4);
  DEFAULT_GATE(5);
  DEFAULT_GATE(6);
  DEFAULT_GATE(7);
  DEFAULT_GATE(8);
  DEFAULT_GATE(9);
  DEFAULT_GATE(10);
  DEFAULT_GATE(11);
  DEFAULT_GATE(12);
  DEFAULT_GATE(13);
  DEFAULT_GATE(14);
  DEFAULT_GATE(15);

  
  //Set all gates to the default handler
  for(int i=16;i<256;i++){
    create_gate(&(idt[i]),default_irq_entry,default_sel,default_attr);
  }

  //Set the timer interrupt (0)
  create_gate(&(idt[8]),timer_irq_entry, default_sel, default_attr);

  //Load IDT
  __asm__ volatile ("lidt %0"
		    :
		    :"m"(idt_reg)
		    );
  
  enable_interrupts();

};

//A union to be able to extract the lower and upper part of an address
union addr_union{
  uint32_t whole;
  struct {
    uint16_t lo16;
    uint16_t hi16;
  };  
};

void IRQ_handler::create_gate(IDTDescr* idt_entry,
			      void (*function_addr)(),
			      uint16_t segment_sel,
			      char attributes
			      )
{
  addr_union addr;
  addr.whole=(uint32_t)function_addr;
  idt_entry->offset_1=addr.lo16;
  idt_entry->offset_2=addr.hi16;
  idt_entry->selector=segment_sel; //TODO: Create memory vars. Private OS-class?
  idt_entry->type_attr=attributes;
  idt_entry->zero=0;  
}

int IRQ_handler::timer_interrupts=0;
static int glob_timer_interrupts=0;


void default_irq_handler(int a, int b, int c){ 
  printf("UNKNOWN IRQ. Args: a=%x, b=%x, c=%x \n\n",a,b,c);     
}  


void timer_irq_handler(){
  glob_timer_interrupts++;
  if(glob_timer_interrupts%5==0){
    printf("\nGot %i timer interrupts \n",
	   glob_timer_interrupts);
  }
  
  return;
    
}  

inline void disable_pic(){
  asm volatile("mov $0xff,%al; "\
	       "out %al,$0xa1; "\
	       "out %al,$0x21; ");
}
