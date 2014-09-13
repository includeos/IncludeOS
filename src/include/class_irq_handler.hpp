#ifndef CLASS_IRQ_HANDLER_H
#define CLASS_IRQ_HANDLER_H

#include "class_os.hpp"
#include "irq/pic_defs.h"


/*
  IDT Type flags  
*/

//Bits 0-3, "type" (VALUES)
const char TRAP_GATE=0xf;
const char TASK_GATE=0x5;
const char INT_GATE=0xe;


//Bit 4, "Storage segment" (BIT NUMBER)
const char BIT_STOR_SEG=0x10; //=0 for trap gates

//Bits 5-6, "Protection level" (BIT NUMBER)
const char BIT_DPL1=0x20;
const char BIT_DPL2=0x40;

//Bit 7, "Present" (BIT NUMBER)
const char BIT_PRESENT=0x80;


//From osdev
struct IDTDescr{
  uint16_t offset_1; // offset bits 0..15
  uint16_t selector; // a code segment selector in GDT or LDT
  uint8_t zero;      // unused, set to 0
  uint8_t type_attr; // type and attributes, see below
  uint16_t offset_2; // offset bits 16..31
};

struct idt_loc{
  uint16_t limit;
  uint32_t base;
}__attribute__ ((packed));



/*
  A class to handle interrupts
 */
class IRQ_handler{
 private:
  static int timer_interrupts;
  static IDTDescr idt[256];
  static void enable_interrupts();
  static const char default_attr=0x8e;
  static const uint16_t default_sel=0x8;
  static bool idt_is_set;
  static void handle_IRQ_default();
  static void create_gate(IDTDescr* idt_entry,
			  void (*function_addr)(),
			  uint16_t segment_sel,
			  char attributes
			  );
 public:
  //Initialize the PIC and load IDT
  static void init();
  
};


#endif
