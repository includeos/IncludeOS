#define NDEBUG // Supress debug

#include <os>
#include <class_irq_handler.hpp>
#include "hw/pic.h"
#include <assert.h>

#define IRQ_BASE 32

bool IRQ_handler::idt_is_set=false;
IDTDescr IRQ_handler::idt[256];
unsigned int IRQ_handler::irq_mask = 0xFFFB; 
irq_bitfield irq_pending = 0;
irq_bitfield IRQ_handler::irq_subscriptions = 0;

void(*IRQ_handler::irq_subscribers[sizeof(irq_bitfield)*8])() = {0};
IRQ_handler::irq_delegate IRQ_handler::irq_delegates[sizeof(irq_bitfield)*8];

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

#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_CMD PIC1
#define PIC2_CMD PIC2
#define PIC_READ_IRR 0x0a    /* OCW3 irq ready next CMD read */
#define PIC_READ_ISR 0x0b    /* OCW3 irq service next CMD read */
#define PIC_EOI 0x20

 
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


/** Default Exception-handler, which just prints its number
 */
#define EXCEPTION_HANDLER(I) \
  void exception_##I##_handler(){ \
    printf("\n\n>>>> !!! CPU EXCEPTION %i !!! <<<<<\n",I);	\
    kill(1,9); \
  }


void eoi2(uint8_t irq){
  if (irq >= 8)
    OS::outb(PIC2_COMMAND,PIC_EOI);
  OS::outb(PIC1_COMMAND,PIC_EOI);
}

/** Atomically increment i.  */
inline void ainc(uint32_t& i){
  __asm__ volatile ("LOCK incl (%0)"::"r"(&i));
}

/** Atomically decrement i. */
inline void adec(uint32_t& i){
  __asm__ volatile ("LOCK decl (%0)"::"r"(&i));
}


/** Default IRQ Handler
    - Set pending flag
    - Increment counter
 */
static uint32_t __irqueues[256]{0};
#define IRQ_HANDLER(I)                                          \
  void irq_##I##_handler(){                                     \
    irq_pending |=  (1 << (I-IRQ_BASE));                        \
    ainc(__irqueues[I-IRQ_BASE]);                               \
    debug("<IRQ !> IRQ %i. Pending: 0x%lx. Count: %li\n",I,    \
           irq_pending,__irqueues[I-IRQ_BASE]);                 \
  }

//debug("<!> IRQ %i. Pending: 0x%lx\n",I,irq_pending);     

// The delegates will handle EOI
// eoi(I-IRQ_BASE);  


  /*
    Macro magic to register default gates
  */  
#define REG_DEFAULT_EXCPT(I) create_gate(&(idt[I]),exception_##I##_entry, \
					default_sel, default_attr );

#define REG_DEFAULT_IRQ(I) create_gate(&(idt[I]),irq_##I##_entry, \
					default_sel, default_attr );


 /* EXCEPTIONS */
#define EXCEPTION_PAIR(I) void exception_##I##_entry(); EXCEPTION_HANDLER(I);
#define IRQ_PAIR(I) void irq_##I##_entry(); IRQ_HANDLER(I);

/*
  IRQ HANDLERS, 
  ! extern: must be visible from assembler
  
  We define two functions for each IRQ/Exception i :

  > void irq/exception_i_entry() - defined in interrupts.s
  > void irq/exception_i_handler() - defined here.
*/
extern "C"{
  void _irq_20_entry(int i);
  //Array of custom IRQ-handlers
  void (*custom_handlers[256])();

  void irq_default_handler();
  void irq_default_entry();

  void irq_timer_entry();
  void irq_timer_handler();


  EXCEPTION_PAIR(0) EXCEPTION_PAIR(1) EXCEPTION_PAIR(2) EXCEPTION_PAIR(3)
  EXCEPTION_PAIR(4) EXCEPTION_PAIR(5) EXCEPTION_PAIR(6) EXCEPTION_PAIR(7)
  EXCEPTION_PAIR(8) EXCEPTION_PAIR(9) EXCEPTION_PAIR(10) EXCEPTION_PAIR(11)
  EXCEPTION_PAIR(12) EXCEPTION_PAIR(13) EXCEPTION_PAIR(14) EXCEPTION_PAIR(15)
  EXCEPTION_PAIR(16) EXCEPTION_PAIR(17) EXCEPTION_PAIR(18) EXCEPTION_PAIR(19)
  EXCEPTION_PAIR(20) /*21-29 Reserved*/ EXCEPTION_PAIR(30) EXCEPTION_PAIR(31);
  
  //Redirected IRQ 0 - 12  
  IRQ_PAIR(32) IRQ_PAIR(33) IRQ_PAIR(34) IRQ_PAIR(35) IRQ_PAIR(36) IRQ_PAIR(37);
  IRQ_PAIR(38) IRQ_PAIR(39) IRQ_PAIR(40) IRQ_PAIR(41) IRQ_PAIR(42) IRQ_PAIR(43);
  
} //End extern

void IRQ_handler::init(){
  //debug("CPU HAS APIC: %s \n", cpuHasAPIC() ? "YES" : "NO" );
  if(idt_is_set){
    printf("ERROR: Trying to reset IDT");
    kill(1,9);
  }
  //Create an idt entry for the 'lidt' instruction
  idt_loc idt_reg;
  idt_reg.limit=(256*sizeof(IDTDescr))-1;
  idt_reg.base=(uint32_t)idt;
  
  printf("\n>>> IRQ handler initializing \n");
    
   //Assign the lower 32 IRQ's : Exceptions
  REG_DEFAULT_EXCPT(0) REG_DEFAULT_EXCPT(1) REG_DEFAULT_EXCPT(2);
  REG_DEFAULT_EXCPT(3) REG_DEFAULT_EXCPT(4) REG_DEFAULT_EXCPT(5);
  REG_DEFAULT_EXCPT(6) REG_DEFAULT_EXCPT(7) REG_DEFAULT_EXCPT(8);
  REG_DEFAULT_EXCPT(9) REG_DEFAULT_EXCPT(10) REG_DEFAULT_EXCPT(11);
  REG_DEFAULT_EXCPT(12) REG_DEFAULT_EXCPT(13) REG_DEFAULT_EXCPT(14);
  REG_DEFAULT_EXCPT(15) REG_DEFAULT_EXCPT(16) REG_DEFAULT_EXCPT(17);
  REG_DEFAULT_EXCPT(18) REG_DEFAULT_EXCPT(19) REG_DEFAULT_EXCPT(20);
  // GATES 21-29 are reserved
  REG_DEFAULT_EXCPT(30) REG_DEFAULT_EXCPT(31);
  
  //Redirected IRQ 0 - 12
  REG_DEFAULT_IRQ(32) REG_DEFAULT_IRQ(33) REG_DEFAULT_IRQ(34);
  REG_DEFAULT_IRQ(35) REG_DEFAULT_IRQ(36) REG_DEFAULT_IRQ(37);
  REG_DEFAULT_IRQ(38) REG_DEFAULT_IRQ(39) REG_DEFAULT_IRQ(40);
  REG_DEFAULT_IRQ(41) REG_DEFAULT_IRQ(42) REG_DEFAULT_IRQ(43);

  
  // Default gates for "real IRQ lines", 32-64
  
  
  printf(" >> Exception gates set for irq < 32 \n");
  
  //Set all irq-gates (>= 44) to the default handler
  for(int i=44;i<256;i++){
    create_gate(&(idt[i]),irq_default_entry,default_sel,default_attr);
  }
  printf(" >> Default interrupt gates set for irq >= 32 \n");
  

  //Load IDT
  __asm__ volatile ("lidt %0": :"m"(idt_reg) );

  //Initialize the interrupt controller
  init_pic();


  //Register the timer and enable / unmask it in the pic
  set_handler(32,irq_timer_entry);
  enable_irq(32); 
    
  enable_irq(33); //Keyboard - now people can subscribe
  enable_interrupts();
  
  //Test zero-division exception
  //int i=0; float x=1/i;  printf("ERROR: 1/0 == %f \n",x);

  //TEST Atomic increment
  uint32_t i=0;
  ainc(i);
  ainc(i);
  ainc(i);
  debug("ATOMIC Increment 3 times: %li \n",i);
  assert(i==3);
  adec(i);
  adec(i);
  adec(i);
  debug("ATOMIC Decrement 3 times: %li \n",i);
  assert(i==0);
  debug("ATOMIC Decrement 3 times: %li \n",i);

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


void IRQ_handler::set_handler(uint8_t irq, void(*function_addr)()){
  create_gate(&idt[irq],function_addr,default_sel,default_attr);
}



static void set_intr_mask(unsigned long mask)
{
  OS::outb(PIC_MSTR_MASK, (unsigned char) mask);
  OS::outb(PIC_SLV_MASK, (unsigned char) (mask >> 8));
}


void IRQ_handler::enable_irq(uint8_t irq){
  printf(">>> Enabling IRQ %i, old mask: 0x%x ",irq,irq_mask);
  irq_mask &= ~(1 << irq);
  if (irq >= 8) irq_mask &= ~(1 << 2);
  set_intr_mask(irq_mask);
  printf(" new mask: 0x%x \n",irq_mask);  
};

int IRQ_handler::timer_interrupts=0;
static int glob_timer_interrupts=0;


/** Let's say we only use 32 IRQ-lines. Then we can use a simple uint32_t
    as bitfield for setting / checking IRQ's. 
*/
void IRQ_handler::subscribe(uint8_t irq, irq_delegate del){   //void(*notify)()
  
  if (irq > sizeof(irq_bitfield)*8)
    panic("Too high IRQ: only IRQ 0 - 32 are subscribable \n");
  
  // Mark IRQ as subscribed to
  irq_subscriptions |= (1 << irq);
  
  // Add callback to subscriber list (for now overwriting any previous)
  //irq_subscribers[irq] = notify;
  irq_delegates[irq] = del;

  
  printf(">>> IRQ subscriptions: 0x%lx irq: 0x%x\n",irq_subscriptions,irq);
}


/** Get most significant bit of b. */
inline int bsr(irq_bitfield b){
  int ret=0;
  __asm__ volatile("bsr %1,%0":"=r"(ret):"r"(b));
  return ret;
}

void IRQ_handler::notify(){

  // Get the IRQ's that are both pending and subscribed to
  irq_bitfield todo = irq_subscriptions & irq_pending;;
  int irq = 0;
  
  while(todo){
    // Select the first IRQ to notify
    irq = bsr(todo);    
    
    // Notify
    debug("<IRQ notify> __irqueue %i Count: %li \n",irq,__irqueues[irq]);
    irq_delegates[irq]();
    
    // Decrement the counter
    adec(__irqueues[irq]);
    
    // Critical section start
    // Spinlock? Well, we can't lock out the IRQ-handler
    // ... and we don't have a timer interrupt so we can't do blocking locks.
    if (!__irqueues[irq]) {
        // Remove the IRQ from pending list            
        irq_pending &= ~(1 << irq);
        //debug("<IRQ notify> IRQ's pending: 0x%lx\n",irq_pending);  
    }
    // Critical section end
    

    // Find remaining IRQ's both pending and subscribed to
    todo = irq_subscriptions & irq_pending;    
  }
  
  //hlt
  __asm__ volatile("hlt;");
}


void IRQ_handler::eoi(uint8_t irq){
  if (irq >= 8)
    OS::outb(PIC2_COMMAND,PIC_EOI);
  OS::outb(PIC1_COMMAND,PIC_EOI);
}

void irq_default_handler(){ 
  // Now we don't really know the IRQ number, 
  // but we can guess by looking at ISR
  uint16_t isr=pic_get_isr();
  //uint16_t irr=pic_get_irr(); //IRR would give us more than we want
  
  printf("\n <IRQ !!!> Unexpected IRQ. ISR: 0x%x. EOI: 0x%x \n",isr,bsr(isr));  
  IRQ_handler::eoi(bsr(isr));
  
}  



void irq_timer_handler(){
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


