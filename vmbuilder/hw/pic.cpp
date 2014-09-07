//
// pic.c
//
// Programmable Interrupt Controller (PIC i8259)
//
// Copyright (C) 2002 Michael Ringgaard. All rights reserved.
//
//  > Ported to IncludeOS by Alfred Bratterud 2014
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 
// 1. Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.  
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.  
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
// SUCH DAMAGE.
// 

#include "pic.h"
#include <os>

// All IRQs disabled initially except cascade

unsigned int irq_mask = 0xFFFB; 

//
// Set interrupt mask
//

static void set_intr_mask(unsigned long mask)
{
  OS::outb(PIC_MSTR_MASK, (unsigned char) mask);
  OS::outb(PIC_SLV_MASK, (unsigned char) (mask >> 8));
}

//
// Initialize the 8259 Programmable Interrupt Controller
//

void init_pic()
{
  OS::outb(PIC_MSTR_CTRL, PIC_MSTR_ICW1);
  OS::outb(PIC_SLV_CTRL, PIC_SLV_ICW1);
  OS::outb(PIC_MSTR_MASK, PIC_MSTR_ICW2);
  OS::outb(PIC_SLV_MASK, PIC_SLV_ICW2);
  OS::outb(PIC_MSTR_MASK, PIC_MSTR_ICW3);
  OS::outb(PIC_SLV_MASK, PIC_SLV_ICW3);
  OS::outb(PIC_MSTR_MASK, PIC_MSTR_ICW4);
  OS::outb(PIC_SLV_MASK, PIC_SLV_ICW4);

  set_intr_mask(irq_mask);
}

//
// Enable IRQ
//

void enable_irq(unsigned int irq)
{
  printf(">>> Enabling IRQ %i, old mask: 0x%x ",irq,irq_mask);
  irq_mask &= ~(1 << irq);
  if (irq >= 8) irq_mask &= ~(1 << 2);
  set_intr_mask(irq_mask);
  printf(" new mask: 0x%x \n",irq_mask);

}

//
// Disable IRQ
//

void disable_irq(unsigned int irq)
{
  irq_mask |= (1 << irq);
  if ((irq_mask & 0xFF00) == 0xFF00) irq_mask |= (1 << 2);
  set_intr_mask(irq_mask);
}

//
// Signal end of interrupt to PIC
//

void eoi(unsigned int irq)
{
  if (irq < 8)
    OS::outb(PIC_MSTR_CTRL, irq + PIC_EOI_BASE);
  else
  {
    OS::outb(PIC_SLV_CTRL, (irq - 8) + PIC_EOI_BASE);
    OS::outb(PIC_MSTR_CTRL, PIC_EOI_CAS);
  }
}
