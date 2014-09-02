#ifndef CLASS_IRQ_HANDLER_H
#define CLASS_IRQ_HANDLER_H

#include "class_os.h"

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
  bool idt_is_set=false;
  static void handle_IRQ_default();
  static void create_gate(IDTDescr* idt_entry,
			  void (*function_addr)(),
			  uint16_t segment_sel,
			  char type,
			  char privilege);
 public:
  static void set_IDT();
  
};


#endif
