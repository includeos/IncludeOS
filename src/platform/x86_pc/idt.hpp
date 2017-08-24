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

#ifndef KERNEL_IDT_HPP
#define KERNEL_IDT_HPP

#include <arch.hpp>

namespace x86
{
#ifdef ARCH_x86_64
// 64-bit
struct IDTDescr {
  uint16_t offset_1;  // offset bits 0..15
  uint16_t selector;  // a code segment selector in GDT or LDT
  uint8_t  ist;       // 3-bit interrupt stack table offset
  uint8_t  type_attr; // type and attributes, see below
  uint16_t offset_2;  // offset bits 16..31
  uint32_t offset_3;  // 32..63
  uint32_t zero2;
};
#else
// 32-bit
struct IDTDescr {
  uint16_t offset_1;  // offset bits 0..15
  uint16_t selector;  // a code segment selector in GDT or LDT
  uint8_t  zero;      // unused, set to 0
  uint8_t  type_attr; // type and attributes, see below
  uint16_t offset_2;  // offset bits 16..31
};
#endif

struct IDTR {
  uint16_t  limit;
  uintptr_t base;
}__attribute__((packed));

extern void idt_initialize_for_cpu(int cpu);
extern void ist_initialize_for_cpu(int cpu, uintptr_t);
}

#endif
