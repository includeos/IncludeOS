#include "kvmclock.hpp"
#include <arch/x86/cpu.hpp>
#include <util/units.hpp>
#include <kernel/cpuid.hpp>
#include <os.hpp>
#include <cstdio>
#include <info>
#include <smp>

using namespace x86;
using namespace util::literals;

struct alignas(4096) pvclock_vcpu_time_info {
	uint32_t version;
	uint32_t pad0;
	uint64_t tsc_timestamp;
	uint64_t system_time;
	uint32_t tsc_to_system_mul;
	signed char tsc_shift;
	unsigned char flags;
	unsigned char pad[2];
}__attribute__((packed));
static SMP::Array<pvclock_vcpu_time_info> vcpu_time;

struct alignas(4096) pvclock_wall_clock {
	uint32_t version;
	uint32_t sec;
	uint32_t nsec;
}__attribute__((packed));
static pvclock_wall_clock kvm_wall_clock;
static uint32_t msr_kvm_wall_clock  = 0;
static uint32_t msr_kvm_system_time = 0;

void KVM_clock::init()
{
  #define MSR_KVM_WALL_CLOCK_NEW  0x4b564d00
  #define MSR_KVM_SYSTEM_TIME_NEW 0x4b564d01

  if (CPUID::kvm_feature(KVM_FEATURE_CLOCKSOURCE2))
  {
    msr_kvm_wall_clock = MSR_KVM_WALL_CLOCK_NEW;
    msr_kvm_system_time = MSR_KVM_SYSTEM_TIME_NEW;
  }
  else if (CPUID::kvm_feature(KVM_FEATURE_CLOCKSOURCE))
  {
    msr_kvm_wall_clock = 0x11;
    msr_kvm_system_time = 0x12;
  }
  else {
    assert(0 && "No KVM clock support detected");
  }

  auto wall_addr = (uintptr_t) &kvm_wall_clock;
  CPU::write_msr(msr_kvm_wall_clock, wall_addr);
  auto vcpu_addr = (uintptr_t) &PER_CPU(vcpu_time);
  CPU::write_msr(msr_kvm_system_time, vcpu_addr | 1);
}
void KVM_clock::deactivate()
{
	CPU::write_msr(msr_kvm_wall_clock, 0);
  CPU::write_msr(msr_kvm_system_time, 0);
}

KHz KVM_clock::get_tsc_khz()
{
  uint64_t khz = 1000000ULL << 32;
  khz /= PER_CPU(vcpu_time).tsc_to_system_mul;

  if (PER_CPU(vcpu_time).tsc_shift < 0)
  	khz <<= -PER_CPU(vcpu_time).tsc_shift;
  else
    khz >>=  PER_CPU(vcpu_time).tsc_shift;
  return KHz(khz);
}

#include "bsd_pvclock.hpp"
uint64_t KVM_clock::system_time()
{
  auto& vcpu = PER_CPU(vcpu_time);
  uint32_t version = 0;
  uint64_t time_ns = 0;
  do
  {
    version = vcpu.version;
    asm("mfence" ::: "memory");
    // nanosecond offset based on TSC
    uint64_t delta = (os::Arch::cpu_cycles() - vcpu.tsc_timestamp);
    time_ns = pvclock_scale_delta(delta, vcpu.tsc_to_system_mul, vcpu.tsc_shift);
    // base system time
    time_ns += vcpu.system_time;
    asm("mfence" ::: "memory");
  }
  while ((vcpu.version & 0x1) || (version != vcpu.version));
  return time_ns;
}

timespec KVM_clock::wall_clock()
{
  uint32_t version = 0;
  timespec tval;
	do
  {
		version = kvm_wall_clock.version;
		asm("mfence" ::: "memory");
		tval.tv_sec  = kvm_wall_clock.sec;
		tval.tv_nsec = kvm_wall_clock.nsec;
		asm("mfence" ::: "memory");
	}
  while ((kvm_wall_clock.version & 1)
      || (kvm_wall_clock.version != version));

  return tval;
}
