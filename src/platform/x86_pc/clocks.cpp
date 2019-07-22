
#include "clocks.hpp"
#include "../kvm/kvmclock.hpp"
#include "cmos_clock.hpp"
#include "platform.hpp"
#include <util/units.hpp>
#include <kernel/cpuid.hpp>
#include <arch.hpp>
#include <delegate>
#include <info>
#include <smp>

using namespace util::literals;

struct sysclock_t
{
  typedef delegate<uint64_t()> system_time_t;
  typedef delegate<timespec()> wall_time_t;
  typedef delegate<KHz()> tsc_khz_t;
  system_time_t system_time = nullptr;
  wall_time_t   wall_time   = nullptr;
  tsc_khz_t     tsc_khz     = nullptr;
};
static sysclock_t current_clock;

namespace x86
{
  void Clocks::init()
  {
    if (0 && CPUID::kvm_feature(KVM_FEATURE_CLOCKSOURCE
                         | KVM_FEATURE_CLOCKSOURCE2))
    {
      KVM_clock::init();
      if (SMP::cpu_id() == 0) {
        current_clock.system_time = {&KVM_clock::system_time};
        current_clock.wall_time   = {&KVM_clock::wall_clock};
        current_clock.tsc_khz     = {&KVM_clock::get_tsc_khz};
        x86::register_deactivation_function(KVM_clock::deactivate);
        INFO("x86", "KVM PV clocks initialized");
      }
    }
    else
    {
      // fallback with CMOS
      if (SMP::cpu_id() == 0) {
        current_clock.system_time = {&CMOS_clock::system_time};
        current_clock.wall_time   = {&CMOS_clock::wall_clock};
        current_clock.tsc_khz     = {&CMOS_clock::get_tsc_khz};
        CMOS_clock::init();
        INFO("x86", "CMOS clock initialized");
      }
    }
  }

  KHz Clocks::get_khz()
  {
    return current_clock.tsc_khz();
  }
}

uint64_t __arch_system_time() noexcept
{
  return current_clock.system_time();
}
timespec __arch_wall_clock() noexcept
{
  return current_clock.wall_time();
}
