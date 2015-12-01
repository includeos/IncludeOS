// Part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef KERNEL_IRQ_MANAGER_HPP
#define KERNEL_IRQ_MANAGER_HPP

#include "os.hpp"
#include "../hw/pic.hpp"
#include <delegate>

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

/** We'll limit the number of subscribable IRQ lines to this.  */
typedef uint32_t irq_bitfield;


//irq_bitfield irq_pending;

extern "C" {
  void irq_default_handler();
}


/** A class to manage interrupt handlers
  
    Anyone can subscribe to IRQ's, but they will be indirectly called via the
    Deferred Procedure Call / Callback system, i.e. when the system is in a 
    wait-loop with nothing else to do.
    
    NOTES:
    * All IRQ-callbacks are in charge of calling End-Of-Interrupt - eoi. 
        Why? Because this makes it possible to prevent further interrupts until 
        a condition of your choice is met. And, interrupts are costly as they 
	always cause vm-exit.
    
    * IRQ-numbering: 0 or 32?
        
        
    @TODO: Remove all dependencies on old SanOS code. In particular, eoi is now in global scope

    
 */
class IRQ_manager{

 public:
  
  typedef delegate<void()> irq_delegate;
  
  /** Enable an IRQ line.  If no handler is set a default will be used
      @param irq : the IRQ to enable
  */
  static void enable_irq(uint8_t irq);
  
  /** Directly set an IRQ handler in IDT.       
      @param irq : The IRQ to handle
      @param function_addr : A proper IRQ handler
      @warning{ 
      This has to be a function that properly returns with `iret`.
      Failure to do so will keep the interrupt from firing and cause a 
      stack overflow or similar badness.
      }
  */
  static void set_handler(uint8_t irq, void(*function_addr)());
  
  /** Get handler from inside the IDT. */
  static void (*get_handler(uint8_t irq))();
  
  /** Subscribe to an IRQ. 
      
      @param irq : the IRQ to subscribe to
	  @param del : a delegate to attach to the IRQ DPC-system, 
      
      The delegate will be called a.s.a.p. after @param irq gets triggered.
      @warning The delegate is responsible for signalling a proper EOI.
      @todo Implies enable_irq(irq)? 
      @todo Create a public member IRQ_manager::eoi for delegates to use
  */
  static void subscribe(uint8_t irq, irq_delegate del);
  
  /** Get the current subscriber of an IRQ-line. 
      @param irq : The IRQ to get subscriber for
  */
  static irq_delegate get_subscriber(uint8_t irq);
  
  /** End of Interrupt. 
      @param irq : The interrupt number
      Indicate to the IRQ-controller that the IRQ is handled, allowing new irq.
      @note Until this is called, no furter IRQ's will be triggered on this line      
      @warning This function is only supposed to be called inside an IRQ-handler   */ 
  static void eoi(uint8_t irq);

  
private:
  static unsigned int irq_mask;
  static int timer_interrupts;
  static IDTDescr idt[256];
  static const char default_attr=0x8e;
  static const uint16_t default_sel=0x8;
  static bool idt_is_set;
  
  /** bit n set means IRQ n has fired since last check */
  //static irq_bitfield irq_pending;
  static irq_bitfield irq_subscriptions;
  
  static void(*irq_subscribers[sizeof(irq_bitfield)*8])();
  static irq_delegate irq_delegates[sizeof(irq_bitfield)*8];
  
  /** STI */
  static void enable_interrupts();
  
  /** @deprecated A default handler */
  static void handle_IRQ_default();
  
  /** Create an IDT-gate. Use "set_handler" for a simpler version 
      using defaults */
  static void create_gate(IDTDescr* idt_entry,
			  void (*function_addr)(),
			  uint16_t segment_sel,
			  char attributes
			  );
  
  /** The OS will call the following : */
  friend class OS;
  friend void ::irq_default_handler();
  
  /** Initialize. Only the OS can initialize the IRQ manager */
  static void init();
  
  /** Notify all delegates waiting for interrupts */
  static void notify();
  

  
};


#endif
