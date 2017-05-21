// This file is a part of the IncludeOS unikernel - www.includeos.org
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

#pragma once
#ifndef HW_APIC_REGS_HPP
#define HW_APIC_REGS_HPP

#define INTR_MASK    0x00010000

#define LAPIC_IRQ_BASE   120
#define LAPIC_IRQ_TIMER  (LAPIC_IRQ_BASE+0)
#define LAPIC_IRQ_LINT0  (LAPIC_IRQ_BASE+3)
#define LAPIC_IRQ_LINT1  (LAPIC_IRQ_BASE+4)
#define LAPIC_IRQ_ERROR  (LAPIC_IRQ_BASE+5)
#define BSP_LAPIC_IPI_IRQ  126

// Interrupt Command Register
#define ICR_DEST_BITS   24

// Delivery Mode
#define ICR_FIXED               0x000000
#define ICR_LOWEST              0x000100
#define ICR_SMI                 0x000200
#define ICR_NMI                 0x000400
#define ICR_INIT                0x000500
#define ICR_STARTUP             0x000600

// Destination Mode
#define ICR_PHYSICAL            0x000000
#define ICR_LOGICAL             0x000800

// Delivery Status
#define ICR_IDLE                0x000000
#define ICR_SEND_PENDING        0x001000
#define ICR_DLV_STATUS          (1u <<12)

// Level
#define ICR_DEASSERT            0x000000
#define ICR_ASSERT              0x004000

// Trigger Mode
#define ICR_EDGE                0x000000
#define ICR_LEVEL               0x008000

// Destination Shorthand
#define ICR_NO_SHORTHAND        0x000000
#define ICR_SELF                0x040000
#define ICR_ALL_INCLUDING_SELF  0x080000
#define ICR_ALL_EXCLUDING_SELF  0x0c0000

#endif
