#ifndef CLASS_IRQ_HANDLER_H
#define CLASS_IRQ_HANDLER_H

#include "class_os.h"

struct idtr_t{
  const char* str;
};




/*
  A class to handle interrupts
 */
class IRQ_handler{
 private:
  bool idt_is_set=false;
  static void handle_IRQ_defualt();
 public:
  static void set_IDT();
};


#endif
