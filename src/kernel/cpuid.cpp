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

#include <kernel/cpuid.hpp>
#include <cstring>
#include <cstdint>
#include <array>

namespace
{
  using namespace CPUID;

  enum class Register { EAX, EBX, ECX, EDX };

  struct FeatureInfo
  {
    // Input when calling cpuid (eax and ecx)
    const uint32_t func;
    const uint32_t subfunc;

    // Register and bit that holds the result
    const Register register_;
    const uint32_t bitmask;
  };

  constexpr auto get_feature_info(Feature f)
  {
    // Use switch-case so that the we get compiler warnings if
    // we forget to add information for features that we have declared
    switch (f)
    {
      // ----------------------------------------------------------------------
      // EAX=1: Processor Info and Feature Bits
      // ----------------------------------------------------------------------
      case Feature::SSE3:         return FeatureInfo { 1, 0, Register::ECX, 1u <<  0 }; // Streaming SIMD Extensions 3
      case Feature::PCLMULQDQ:    return FeatureInfo { 1, 0, Register::ECX, 2u <<  0 }; // PCLMULQDQ Instruction
      case Feature::DTES64:       return FeatureInfo { 1, 0, Register::ECX, 1u <<  2 }; // 64-Bit Debug Store Area
      case Feature::MONITOR:      return FeatureInfo { 1, 0, Register::ECX, 1u <<  3 }; // MONITOR/MWAIT
      case Feature::DS_CPL:       return FeatureInfo { 1, 0, Register::ECX, 1u <<  4 }; // CPL Qualified Debug Store
      case Feature::VMX:          return FeatureInfo { 1, 0, Register::ECX, 1u <<  5 }; // Virtual Machine Extensions
      case Feature::SMX:          return FeatureInfo { 1, 0, Register::ECX, 1u <<  6 }; // Safer Mode Extensions (Intel TXT)
      case Feature::EST:          return FeatureInfo { 1, 0, Register::ECX, 1u <<  7 }; // Enhanced SpeedStep Technology
      case Feature::TM2:          return FeatureInfo { 1, 0, Register::ECX, 1u <<  8 }; // Thermal Monitor 2
      case Feature::SSSE3:        return FeatureInfo { 1, 0, Register::ECX, 1u <<  9 }; // Supplemental Streaming SIMD Extensions 3
      case Feature::CNXT_ID:      return FeatureInfo { 1, 0, Register::ECX, 1u << 10 }; // L1 Context ID
      case Feature::FMA:          return FeatureInfo { 1, 0, Register::ECX, 1u << 12 }; // Fused Multiply Add
      case Feature::CX16:         return FeatureInfo { 1, 0, Register::ECX, 1u << 13 }; // CMPXCHG16B Instruction
      case Feature::XTPR:         return FeatureInfo { 1, 0, Register::ECX, 1u << 14 }; // xTPR Update Control
      case Feature::PDCM:         return FeatureInfo { 1, 0, Register::ECX, 1u << 15 }; // Perf/Debug Capability MSR
      case Feature::PCID:         return FeatureInfo { 1, 0, Register::ECX, 1u << 17 }; // Process-context Identifiers
      case Feature::DCA:          return FeatureInfo { 1, 0, Register::ECX, 1u << 18 }; // Direct Cache Access
      case Feature::SSE4_1:       return FeatureInfo { 1, 0, Register::ECX, 1u << 19 }; // Streaming SIMD Extensions 4.1
      case Feature::SSE4_2:       return FeatureInfo { 1, 0, Register::ECX, 1u << 20 }; // Streaming SIMD Extensions 4.2
      case Feature::X2APIC:       return FeatureInfo { 1, 0, Register::ECX, 1u << 21 }; // Extended xAPIC Support
      case Feature::MOVBE:        return FeatureInfo { 1, 0, Register::ECX, 1u << 22 }; // MOVBE Instruction
      case Feature::POPCNT:       return FeatureInfo { 1, 0, Register::ECX, 1u << 23 }; // POPCNT Instruction
      case Feature::TSC_DEADLINE: return FeatureInfo { 1, 0, Register::ECX, 1u << 24 }; // Local APIC supports TSC Deadline
      case Feature::AES:          return FeatureInfo { 1, 0, Register::ECX, 1u << 25 }; // AESNI Instruction
      case Feature::XSAVE:        return FeatureInfo { 1, 0, Register::ECX, 1u << 26 }; // XSAVE/XSTOR States
      case Feature::OSXSAVE:      return FeatureInfo { 1, 0, Register::ECX, 1u << 27 }; // OS Enabled Extended State Management
      case Feature::AVX:          return FeatureInfo { 1, 0, Register::ECX, 1u << 28 }; // AVX Instructions
      case Feature::F16C:         return FeatureInfo { 1, 0, Register::ECX, 1u << 29 }; // 16-bit Floating Point Instructions
      case Feature::RDRAND:       return FeatureInfo { 1, 0, Register::ECX, 1u << 30 }; // RDRAND Instruction

      case Feature::FPU:          return FeatureInfo { 1, 0, Register::EDX, 1u <<  0 }; // Floating-Point Unit On-Chip
      case Feature::VME:          return FeatureInfo { 1, 0, Register::EDX, 1u <<  1 }; // Virtual 8086 Mode Extensions
      case Feature::DE:           return FeatureInfo { 1, 0, Register::EDX, 1u <<  2 }; // Debugging Extensions
      case Feature::PSE:          return FeatureInfo { 1, 0, Register::EDX, 1u <<  3 }; // Page Size Extension
      case Feature::TSC:          return FeatureInfo { 1, 0, Register::EDX, 1u <<  4 }; // Time Stamp Counter
      case Feature::MSR:          return FeatureInfo { 1, 0, Register::EDX, 1u <<  5 }; // Model Specific Registers
      case Feature::PAE:          return FeatureInfo { 1, 0, Register::EDX, 1u <<  6 }; // Physical Address Extension
      case Feature::MCE:          return FeatureInfo { 1, 0, Register::EDX, 1u <<  7 }; // Machine-Check Exception
      case Feature::CX8:          return FeatureInfo { 1, 0, Register::EDX, 1u <<  8 }; // CMPXCHG8 Instruction
      case Feature::APIC:         return FeatureInfo { 1, 0, Register::EDX, 1u <<  9 }; // APIC On-Chip
      case Feature::SEP:          return FeatureInfo { 1, 0, Register::EDX, 1u << 11 }; // SYSENTER/SYSEXIT instructions
      case Feature::MTRR:         return FeatureInfo { 1, 0, Register::EDX, 1u << 12 }; // Memory Type Range Registers
      case Feature::PGE:          return FeatureInfo { 1, 0, Register::EDX, 1u << 13 }; // Page Global Bit
      case Feature::MCA:          return FeatureInfo { 1, 0, Register::EDX, 1u << 14 }; // Machine-Check Architecture
      case Feature::CMOV:         return FeatureInfo { 1, 0, Register::EDX, 1u << 15 }; // Conditional Move Instruction
      case Feature::PAT:          return FeatureInfo { 1, 0, Register::EDX, 1u << 16 }; // Page Attribute Table
      case Feature::PSE_36:       return FeatureInfo { 1, 0, Register::EDX, 1u << 17 }; // 36-bit Page Size Extension
      case Feature::PSN:          return FeatureInfo { 1, 0, Register::EDX, 1u << 18 }; // Processor Serial Number
      case Feature::CLFLUSH:      return FeatureInfo { 1, 0, Register::EDX, 1u << 19 }; // CLFLUSH Instruction
      case Feature::DS:           return FeatureInfo { 1, 0, Register::EDX, 1u << 21 }; // Debug Store
      case Feature::ACPI:         return FeatureInfo { 1, 0, Register::EDX, 1u << 22 }; // Thermal Monitor and Software Clock Facilities
      case Feature::MMX:          return FeatureInfo { 1, 0, Register::EDX, 1u << 23 }; // MMX Technology
      case Feature::FXSR:         return FeatureInfo { 1, 0, Register::EDX, 1u << 24 }; // FXSAVE and FXSTOR Instructions
      case Feature::SSE:          return FeatureInfo { 1, 0, Register::EDX, 1u << 25 }; // Streaming SIMD Extensions
      case Feature::SSE2:         return FeatureInfo { 1, 0, Register::EDX, 1u << 26 }; // Streaming SIMD Extensions 2
      case Feature::SS:           return FeatureInfo { 1, 0, Register::EDX, 1u << 27 }; // Self Snoop
      case Feature::HTT:          return FeatureInfo { 1, 0, Register::EDX, 1u << 28 }; // Multi-Threading
      case Feature::TM:           return FeatureInfo { 1, 0, Register::EDX, 1u << 29 }; // Thermal Monitor
      case Feature::PBE:          return FeatureInfo { 1, 0, Register::EDX, 1u << 31 }; // Pending Break Enable

      // ----------------------------------------------------------------------
      // EAX=80000001h: Extended Processor Info and Feature Bits (not complete)
      // ----------------------------------------------------------------------
      case Feature::SYSCALL:      return FeatureInfo { 0x80000001, 0, Register::EDX, 1u << 11 }; // SYSCALL/SYSRET
      case Feature::NX:           return FeatureInfo { 0x80000001, 0, Register::EDX, 1u << 20 }; // Execute Disable Bit
      case Feature::PDPE1GB:      return FeatureInfo { 0x80000001, 0, Register::EDX, 1u << 26 }; // 1 GB Pages
      case Feature::RDTSCP:       return FeatureInfo { 0x80000001, 0, Register::EDX, 1u << 27 }; // RDTSCP and IA32_TSC_AUX
      case Feature::LM:           return FeatureInfo { 0x80000001, 0, Register::EDX, 1u << 29 }; // 64-bit Architecture

      case Feature::SVM:          return FeatureInfo { 0x80000001, 0, Register::ECX, 1u <<  2 }; // Secure Virtual Machine (AMD-V)
      case Feature::SSE4A:        return FeatureInfo { 0x80000001, 0, Register::ECX, 1u <<  6 }; // SSE4a
    }
  }

  // Holds results of call to cpuid
  struct cpuid_t
  {
    uint32_t EAX;
    uint32_t EBX;
    uint32_t ECX;
    uint32_t EDX;
  }; //< cpuid_t

  cpuid_t cpuid(uint32_t func, uint32_t subfunc)
  {
    // Cache up to 4 results
    struct cpuid_cache_t
    {
      // cpuid input
      uint32_t func;
      uint32_t subfunc;

      // cpuid output
      cpuid_t result;

      // valid or not in cache
      bool valid;
    };
    static std::array<cpuid_cache_t, 4> cache = {};

    // Check cache for cpuid result
    for (const auto& cached : cache)
    {
      if (cached.valid &&
          cached.func == func &&
          cached.subfunc == subfunc)
      {
        return cached.result;
      }
    }

    // Call cpuid
    // EBX/RBX needs to be preserved depending on the memory model and use of PIC
    cpuid_t result;
    asm volatile ("cpuid"
      : "=a"(result.EAX), "=b"(result.EBX), "=c"(result.ECX), "=d"(result.EDX)
      : "a"(func), "c"(subfunc) : "%eax", "%ebx", "%ecx", "%edx");

    // Try to find an empty spot in the cache
    for (auto& cached : cache)
    {
      if (!cached.valid)
      {
        cached.func = func;
        cached.subfunc = subfunc;
        cached.result = result;
        cached.valid = true;
        break;
      }
    }

    return result;
  }

} //< namespace

bool CPUID::is_amd_cpu()
{
  auto result = cpuid(0, 0);
  return
     memcmp(reinterpret_cast<char*>(&result.EBX), "htuA", 4) == 0
  && memcmp(reinterpret_cast<char*>(&result.EDX), "itne", 4) == 0
  && memcmp(reinterpret_cast<char*>(&result.ECX), "DMAc", 4) == 0;
}

bool CPUID::is_intel_cpu()
{
  auto result = cpuid(0, 0);
  return
     memcmp(reinterpret_cast<char*>(&result.EBX), "Genu", 4) == 0
  && memcmp(reinterpret_cast<char*>(&result.EDX), "ineI", 4) == 0
  && memcmp(reinterpret_cast<char*>(&result.ECX), "ntel", 4) == 0;
}

bool CPUID::has_feature(Feature f)
{
  const auto feature_info = get_feature_info(f);
  const auto cpuid_result = cpuid(feature_info.func, feature_info.subfunc);

  switch (feature_info.register_)
  {
    case Register::EAX: return (cpuid_result.EAX & feature_info.bitmask) != 0;
    case Register::EBX: return (cpuid_result.EBX & feature_info.bitmask) != 0;
    case Register::ECX: return (cpuid_result.ECX & feature_info.bitmask) != 0;
    case Register::EDX: return (cpuid_result.EDX & feature_info.bitmask) != 0;
  }
}

#define KVM_CPUID_SIGNATURE       0x40000000

unsigned CPUID::kvm_function()
{
  auto res = cpuid(KVM_CPUID_SIGNATURE, 0);
  /// "KVMKVMKVM"
  if (res.EBX == 0x4b4d564b && res.ECX == 0x564b4d56 && res.EDX == 0x4d)
      return res.EAX;
  return 0;
}
bool CPUID::kvm_feature(unsigned id)
{
  unsigned func = kvm_function();
  if (func == 0) return false;
  auto res = cpuid(func, 0);
  return (res.EAX & (1 << id)) != 0;
}
