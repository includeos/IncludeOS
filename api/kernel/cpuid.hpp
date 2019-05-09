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
#ifndef KERNEL_CPUID_HPP
#define KERNEL_CPUID_HPP

#include <unordered_map>
#include <vector>

namespace CPUID
{

  enum class Feature
  {
    // ------------------------------------------------------------------------
    // Processor Info and Feature Bits
    // ------------------------------------------------------------------------
    SSE3,              // Streaming SIMD Extensions 3
    PCLMULQDQ,         // PCLMULQDQ Instruction
    DTES64,            // 64-Bit Debug Store Area
    MONITOR,           // MONITOR/MWAIT
    DS_CPL,            // CPL Qualified Debug Store
    VMX,               // Virtual Machine Extensions (VT-x)
    SMX,               // Safer Mode Extensions
    EST,               // Enhanced SpeedStep Technology
    TM2,               // Thermal Monitor 2
    SSSE3,             // Supplemental Streaming SIMD Extensions 3
    CNXT_ID,           // L1 Context ID
    FMA,               // Fused Multiply Add
    CX16,              // CMPXCHG16B Instruction
    XTPR,              // xTPR Update Control
    PDCM,              // Perf/Debug Capability MSR
    PCID,              // Process-context Identifiers
    DCA,               // Direct Cache Access
    SSE4_1,            // Streaming SIMD Extensions 4.1
    SSE4_2,            // Streaming SIMD Extensions 4.2
    X2APIC,            // Extended xAPIC Support
    MOVBE,             // MOVBE Instruction (move after swapping bytes)
    POPCNT,            // POPCNT Instruction
    TSC_DEADLINE,      // Local APIC supports TSC Deadline
    AES,               // AESNI Instruction
    XSAVE,             // XSAVE/XSTOR States
    OSXSAVE,           // OS Enabled Extended State Management
    AVX,               // AVX Instructions
    F16C,              // 16-bit Floating Point Instructions
    RDRAND,            // RDRAND Instruction
    RDSEED,            // RDSEED Instruction

    FPU,               // Floating-Point Unit On-Chip
    VME,               // Virtual 8086 Mode Extensions
    DE,                // Debugging Extensions
    PSE,               // Page Size Extension
    TSC,               // Time Stamp Counter
    MSR,               // Model Specific Registers
    PAE,               // Physical Address Extension
    MCE,               // Machine-Check Exception
    CX8,               // CMPXCHG8 Instruction
    APIC,              // APIC On-Chip
    SEP,               // SYSENTER/SYSEXIT instructions
    MTRR,              // Memory Type Range Registers
    PGE,               // Page Global Bit
    MCA,               // Machine-Check Architecture
    CMOV,              // Conditional Move Instruction
    PAT,               // Page Attribute Table
    PSE_36,            // 36-bit Page Size Extension
    PSN,               // Processor Serial Number
    CLFLUSH,           // CLFLUSH Instruction
    DS,                // Debug Store
    ACPI,              // Thermal Monitor and Software Clock Facilities
    MMX,               // MMX Technology
    FXSR,              // FXSAVE and FXSTOR Instructions
    SSE,               // Streaming SIMD Extensions
    SSE2,              // Streaming SIMD Extensions 2
    SS,                // Self Snoop
    HTT,               // Multi-Threading
    TM,                // Thermal Monitor
    PBE,               // Pending Break Enable

    // ------------------------------------------------------------------------
    // Extended Processor Info and Feature Bits (not complete)
    // ------------------------------------------------------------------------
    SYSCALL,           // SYSCALL/SYSRET
    NX,                // Execute Disable Bit
    PDPE1GB,           // 1 GB Pages
    RDTSCP,            // RDTSCP and IA32_TSC_AUX
    LM,                // Long mode (64-bit Architecture)

    SVM,               // Secture Virtual Machines (AMD-V)
                       // aka. AMD-V (AMD's virtualization extension)
    SSE4A,             // SSE4a

    // ------------------------------------------------------------------------
    // 4th gen. Core features
    // ------------------------------------------------------------------------
    AVX2,              // AVX2
    BMI1,              // Bit manipulation 1
    BMI2,              // Bit manipulation 2
    LZCNT,             // Count leading zero bits

    TSC_INV,           // Invariant TSC
  };

  std::vector<const char*> detect_features_str();
  std::vector<Feature>     detect_features();

  bool is_amd_cpu()   noexcept;
  bool is_intel_cpu() noexcept;
  bool has_feature(Feature f);

  bool kvm_feature(unsigned mask) noexcept;
} //< CPUID



#define KVM_FEATURE_CLOCKSOURCE   0x1
#define KVM_FEATURE_NOP_IO_DELAY  0x2
#define KVM_FEATURE_MMU_OP        0x4   /* deprecated */
#define KVM_FEATURE_CLOCKSOURCE2  0x8
#define KVM_FEATURE_ASYNC_PF      0x10
#define KVM_FEATURE_STEAL_TIME    0x20
#define KVM_FEATURE_PV_EOI        0x40
#define KVM_FEATURE_PV_UNHALT     0x80
#define KVM_FEATURE_CLOCKSOURCE_STABLE_BIT 24

#endif //< KERNEL_CPUID_HPP
