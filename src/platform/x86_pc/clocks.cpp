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

#include "clocks.hpp"
#include "../kvm/kvmclock.hpp"
#include "cmos_clock.hpp"
#include <kernel/cpuid.hpp>
#include <arch.hpp>
#include <delegate>
#include <info>
#include <smp>

struct sysclock_t
{
  typedef delegate<uint64_t()> system_time_t;
  system_time_t system_time = nullptr;
  system_time_t wall_time   = nullptr;
};
static SMP_ARRAY<sysclock_t> vcpu_clock;

namespace x86
{
  void Clocks::init()
  {
    if (CPUID::kvm_feature(KVM_FEATURE_CLOCKSOURCE2))
    {
      KVM_clock::init();
      PER_CPU(vcpu_clock).system_time = {&KVM_clock::system_time};
      PER_CPU(vcpu_clock).wall_time   = {&KVM_clock::wall_clock};
    }
    else
    {
      // fallback with CMOS
      if (SMP::cpu_id() == 0) CMOS_clock::init();
      PER_CPU(vcpu_clock).system_time = {&CMOS_clock::system_time};
      PER_CPU(vcpu_clock).wall_time   = {&CMOS_clock::wall_clock};
    }
  }
}

uint64_t __arch_system_time() noexcept
{
  return PER_CPU(vcpu_clock).system_time();
}
uint64_t __arch_wall_clock() noexcept
{
  return PER_CPU(vcpu_clock).wall_time();
}
