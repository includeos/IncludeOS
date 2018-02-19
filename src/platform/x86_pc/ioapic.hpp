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
#ifndef X86_IOAPIC_HPP
#define X86_IOAPIC_HPP

#include <cstdint>
#include "acpi.hpp" // ACPI

#define IOAPIC_MAX_REDIRENTS (1 << 16)

// low 32 IRQ entry
#define IOAPIC_MODE_FIXED   0x0
#define IOAPIC_MODE_LOWPRI  0x1
#define IOAPIC_MODE_SMI     0x2
#define IOAPIC_MODE_NMI     0x4
#define IOAPIC_MODE_INIT    0x5
#define IOAPIC_MODE_ExtINT  0x7

#define IOAPIC_DEST_PHYSICAL   0
#define IOAPIC_DEST_LOGICAL    (1 << 11)

#define IOAPIC_STATUS_RELAXED  0
#define IOAPIC_STATUS_SENT     (1 << 12)

#define IOAPIC_PIN_HIGH        0
#define IOAPIC_PIN_LOW         (1 << 13)

#define IOAPIC_TRIGGER_EDGE    0
#define IOAPIC_TRIGGER_LEVEL   (1 << 15)
#define IOAPIC_IRQ_DISABLE     (1 << 16)

// high 32 IRQ entry
#define IOAPIC_HI_DESTINATION  (1 << 24)

namespace x86 {

  class IOAPIC {
  public:
    static void init(const ACPI::ioapic_list& vec);

    static void enable(uint8_t lapic, const ACPI::override_t&);
    static void disable(uint8_t idx);
  };

}

#endif
