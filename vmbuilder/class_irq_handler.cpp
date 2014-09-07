#include "class_irq_handler.h"
#include "hw/pic.h"

bool IRQ_handler::idt_is_set=false;
IDTDescr IRQ_handler::idt[256];

void IRQ_handler::enable_interrupts(){
  __asm__ volatile("sti");
}

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

#define PIC1_CMD                    0x20
#define PIC2_CMD                    0xA0
#define PIC_READ_IRR                0x0a    /* OCW3 irq ready next CMD read */
#define PIC_READ_ISR                0x0b    /* OCW3 irq service next CMD read */
 
/* Helper func */
static uint16_t __pic_get_irq_reg(int ocw3)
{
  /* OCW3 to PIC CMD to get the register values.  PIC2 is chained, and
   * represents IRQs 8-15.  PIC1 is IRQs 0-7, with 2 being the chain */
  OS::outb(PIC1_CMD, ocw3);
  OS::outb(PIC2_CMD, ocw3);
  return (OS::inb(PIC2_CMD) << 8) | OS::inb(PIC1_CMD);
}
 
/* Returns the combined value of the cascaded PICs irq request register */
uint16_t pic_get_irr(void)
{
  return __pic_get_irq_reg(PIC_READ_IRR);
}
 
/* Returns the combined value of the cascaded PICs in-service register */
uint16_t pic_get_isr(void)
{
  return __pic_get_irq_reg(PIC_READ_ISR);
}


  /*
    Macro for a default IRQ-handler, which just prints its number
   */
#define EXCEPTION_HANDLER(I) \
  void exception_##I##_handler(){ \
    printf("CPU EXCEPTION %i \n",I);  \
    kill(1,9); \
  }
  
  /*
    Macro magic to register default gates
  */  
#define REG_DEFAULT_GATE(I) create_gate(&(idt[I]),exception_##I##_entry, \
					default_sel, default_attr );

/*
  IRQ HANDLERS, 
  ! extern: must be visible from assembler
  
  We define two functions for each IRQ/Exception i :

  > void irq/exception_i_entry() - defined in interrupts.s
  > void irq/exception_i_handler() - defined here.
*/
extern "C"{
  void default_irq_handler();
  void default_irq_entry();

  void timer_irq_entry();
  void timer_irq_handler();

 /* EXCEPTIONS */
#define EXCEPTION_PAIR(I) void exception_##I##_entry(); EXCEPTION_HANDLER(I);
  EXCEPTION_PAIR(0);
  EXCEPTION_PAIR(1);
  EXCEPTION_PAIR(2);
  EXCEPTION_PAIR(3);
  EXCEPTION_PAIR(4);
  EXCEPTION_PAIR(5);
  EXCEPTION_PAIR(6);
  EXCEPTION_PAIR(7);
  EXCEPTION_PAIR(8);
  EXCEPTION_PAIR(9);
  EXCEPTION_PAIR(10);
  EXCEPTION_PAIR(11);
  EXCEPTION_PAIR(12);
  EXCEPTION_PAIR(13);
  EXCEPTION_PAIR(14);
  EXCEPTION_PAIR(15);
  EXCEPTION_PAIR(16);
  EXCEPTION_PAIR(17);
  EXCEPTION_PAIR(18);
  EXCEPTION_PAIR(19);
  EXCEPTION_PAIR(20);
//EXCEPTION 21 - 29 are reserved
  EXCEPTION_PAIR(30);
  EXCEPTION_PAIR(31);

} //End extern

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
    
  //Set all gates to the default handler
  //for(int i=16;i<256;i++){
  for(int i=32;i<256;i++){
    create_gate(&(idt[i]),default_irq_entry,default_sel,default_attr);
  }
  
  //Assign the lower 32 IRQ's : Exceptions
  REG_DEFAULT_GATE(0);
  REG_DEFAULT_GATE(1);
  REG_DEFAULT_GATE(2);
  REG_DEFAULT_GATE(3);
  REG_DEFAULT_GATE(4);
  REG_DEFAULT_GATE(5);
  REG_DEFAULT_GATE(6);
  REG_DEFAULT_GATE(7);
  REG_DEFAULT_GATE(8);
  REG_DEFAULT_GATE(9);
  REG_DEFAULT_GATE(10);
  REG_DEFAULT_GATE(11);
  REG_DEFAULT_GATE(12);
  REG_DEFAULT_GATE(13);
  REG_DEFAULT_GATE(14);
  REG_DEFAULT_GATE(15);
  REG_DEFAULT_GATE(16);
  REG_DEFAULT_GATE(17);
  REG_DEFAULT_GATE(18);
  REG_DEFAULT_GATE(19);
  REG_DEFAULT_GATE(20);
  // GATES 21-29 are reserved
  REG_DEFAULT_GATE(30);
  REG_DEFAULT_GATE(31);

  //Register the timer 
  //create_gate(&(idt[32]),timer_irq_entry, default_sel, default_attr);

  //Load IDT
  __asm__ volatile ("lidt %0": :"m"(idt_reg) );

  //Initialize the interrupt controller
  init_pic();
  
  //enable_irq(32); //Timer
  enable_irq(33); //Keyboard
  //enable_interrupts();
  
  //Test zero-division exception
  //int i=0; float x=1/i;  printf("ERROR: 1/0 == %f \n",x);
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


void default_irq_handler(){ 
  uint16_t irr=pic_get_irr();
  uint16_t isr=pic_get_irr();
  printf("UNEXPECTED IRQ: ISR: %i, IRR: %i \n",isr,irr);     
}  

void timer_irq_handler(){
  glob_timer_interrupts++;
  if(glob_timer_interrupts%16==0){
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
