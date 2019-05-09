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
#include <unordered_map>

namespace std
{
  template<>
  struct hash<CPUID::Feature> {
    size_t operator()(const CPUID::Feature& f) const {
      return std::hash<int>()(static_cast<size_t>(f));
    }
  };
}

namespace CPUID {
  const std::unordered_map<Feature, const char*> feature_names
  {
      {Feature::SSE2,"SSE2"},
      {Feature::SSE3,"SSE3"},
      {Feature::SSSE3,"SSSE3"},
      {Feature::RDRAND,"RDRAND"},
      {Feature::RDSEED,"RDSEED"},
      {Feature::XSAVE,"XSAVE"},
      {Feature::FXSR,"FXSR"},
      {Feature::AES,"AES"},
      {Feature::AVX,"AVX"},
      {Feature::SSE4A,"SSE4a"},
      {Feature::SSE4_1,"SSE4.1"},
      {Feature::SSE4_2,"SSE4.2"},
      {Feature::NX,"NX"},
      {Feature::SYSCALL,"SYSCALL"},
      {Feature::PDPE1GB,"PDPE1GB"},
      {Feature::RDTSCP,"RDTSCP"},
      {Feature::FMA, "FMA"},
      {Feature::AVX2, "AVX2"},
      {Feature::BMI1,"BMI1"},
      {Feature::BMI2,"BMI2"},
      {Feature::LZCNT,"LZCNT"},
      {Feature::MOVBE,"MOVBE"},
      {Feature::PCLMULQDQ,"PCLMULQDQ"},
      {Feature::DTES64,"DTES64"},
      {Feature::MONITOR,"MONITOR"},
      {Feature::DS_CPL,"DS_CPL"},
      {Feature::VMX,"VMX"},
      {Feature::SMX,"SMX"},
      {Feature::EST,"EST"},
      {Feature::TM2,"TM2"},
      {Feature::CNXT_ID,"CNXT_ID"},
      {Feature::CX16,"CX16"},
      {Feature::XTPR,"XTPR"},
      {Feature::PDCM,"PDCM"},
      {Feature::PCID,"PCID"},
      {Feature::DCA,"DCA"},
      {Feature::X2APIC,"X2APIC"},
      {Feature::POPCNT,"POPCNT"},
      {Feature::TSC_DEADLINE,"TSC_DEADLINE"},
      {Feature::OSXSAVE,"OSXSAVE"},
      {Feature::F16C,"F16C"},
      {Feature::FPU,"FPU"},
      {Feature::VME,"VME"},
      {Feature::DE,"DE"},
      {Feature::PSE,"PSE"},
      {Feature::TSC,"TSC"},
      {Feature::MSR,"MSR"},
      {Feature::PAE,"PAE"},
      {Feature::MCE,"MCE"},
      {Feature::CX8,"CX8"},
      {Feature::APIC,"APIC"},
      {Feature::SEP,"SEP"},
      {Feature::MTRR,"MTRR"},
      {Feature::PGE,"PGE"},
      {Feature::MCA,"MCA"},
      {Feature::CMOV,"CMOV"},
      {Feature::PAT,"PAT"},
      {Feature::PSE_36,"PSE_36"},
      {Feature::PSN,"PSN"},
      {Feature::CLFLUSH,"CLFLUSH"},
      {Feature::DS,"DS"},
      {Feature::ACPI,"ACPI"},
      {Feature::MMX,"MMX"},
      {Feature::FXSR,"FXSR"},
      {Feature::SSE,"SSE"},
      {Feature::SS,"SS"},
      {Feature::HTT,"HTT"},
      {Feature::TM,"TM"},
      {Feature::NX,"NX"},
      {Feature::PDPE1GB,"PDPE1GB"},
      {Feature::RDTSCP,"RDTSCP"},
      {Feature::LM,"LM"},
      {Feature::SVM,"SVM"},
      {Feature::TSC_INV,"TSC_INV"}
  };
}

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
      case Feature::TSC_INV:      return FeatureInfo { 0x80000007, 0, Register::EDX, 1u << 8 };  // Invariant TSC

      case Feature::SVM:          return FeatureInfo { 0x80000001, 0, Register::ECX, 1u <<  2 }; // Secure Virtual Machine (AMD-V)
      case Feature::SSE4A:        return FeatureInfo { 0x80000001, 0, Register::ECX, 1u <<  6 }; // SSE4a
      // Standard function 7
      case Feature::AVX2:         return FeatureInfo { 7, 0, Register::ECX, 1u <<  5 }; // AVX2
      case Feature::BMI1:         return FeatureInfo { 7, 0, Register::ECX, 1u <<  3 }; // BMI1
      case Feature::BMI2:         return FeatureInfo { 7, 0, Register::ECX, 1u <<  8 }; // BMI2
      case Feature::LZCNT:        return FeatureInfo { 7, 0, Register::ECX, 1u <<  5 }; // LZCNT
      case Feature::RDSEED:       return FeatureInfo { 7, 0, Register::EBX, 1u << 18 }; // RDSEED
      default: throw std::out_of_range("Unimplemented CPU feature encountered");
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

  static cpuid_t cpuid(uint32_t func, uint32_t subfunc)
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
#if defined(ARCH_x86) || defined(ARCH_x86_64)
    asm volatile ("cpuid"
      : "=a"(result.EAX), "=b"(result.EBX), "=c"(result.ECX), "=d"(result.EDX)
      : "a"(func), "c"(subfunc));
#elif defined(ARCH_aarch64)

#endif
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

bool CPUID::is_amd_cpu() noexcept
{
  auto result = cpuid(0, 0);
  return
     memcmp((char*) &result.EBX, "Auth", 4) == 0
  && memcmp((char*) &result.EDX, "enti", 4) == 0
  && memcmp((char*) &result.ECX, "cAMD", 4) == 0;
}

bool CPUID::is_intel_cpu() noexcept
{
  auto result = cpuid(0, 0);
  return
     memcmp((char*) &result.EBX, "Genu", 4) == 0
  && memcmp((char*) &result.EDX, "ineI", 4) == 0
  && memcmp((char*) &result.ECX, "ntel", 4) == 0;
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
  return false;
}

#define KVM_CPUID_SIGNATURE       0x40000000

static unsigned kvm_function() noexcept
{
  auto res = cpuid(KVM_CPUID_SIGNATURE, 0);
  /// "KVMKVMKVM"
  if (res.EBX == 0x4b4d564b && res.ECX == 0x564b4d56 && res.EDX == 0x4d)
      return res.EAX;
  return 0;
}
bool CPUID::kvm_feature(unsigned mask) noexcept
{
  unsigned func = kvm_function();
  if (func == 0) return false;
  auto res = cpuid(func, 0);
  return (res.EAX & mask) != 0;
}

std::vector<Feature> CPUID::detect_features() {
  std::vector<Feature> vec;
  for (const auto feat : feature_names) {
    if (CPUID::has_feature(feat.first))
      vec.push_back(feat.first);
  }
  return vec;
}

std::vector<const char*> CPUID::detect_features_str() {
  std::vector<const char*> names;
  auto features = detect_features();
  for (auto& feat : features) {
    names.push_back(feature_names.at(feat));
  }
  return names;
}
