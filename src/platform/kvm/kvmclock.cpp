#include "kvmclock.hpp"
#include "../x86_pc/cpu.hpp"
#include <cstdio>
#include <info>
#include <smp>
#define MSR_KVM_WALL_CLOCK_NEW  0x4b564d00
#define MSR_KVM_SYSTEM_TIME_NEW 0x4b564d01
using namespace x86;

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
static SMP_ARRAY<pvclock_vcpu_time_info> vcpu_time;

struct pvclock_wall_clock {
	uint32_t version;
	uint32_t sec;
	uint32_t nsec;
}__attribute__((__packed__));
static pvclock_wall_clock kvm_wall_clock;

void KVM_clock::init()
{
  //auto wall_addr = (uintptr_t) &wall_clock;
  //CPU::write_msr(MSR_KVM_WALL_CLOCK_NEW, wall_addr);
  auto vcpu_addr = (uintptr_t) &PER_CPU(vcpu_time);
  CPU::write_msr(MSR_KVM_SYSTEM_TIME_NEW, vcpu_addr | 1);
  INFO("KVM", "KVM clocks initialized");
}

uint64_t KVM_clock::system_time()
{
  auto& vcpu = PER_CPU(vcpu_time);
  uint32_t version = 0;
  uint64_t time_ns = 0;
  do
  {
    version = vcpu.version;
    asm("mfence" ::: "memory");
    time_ns = (__arch_cpu_cycles() - vcpu.tsc_timestamp);
    if (vcpu.tsc_shift >= 0) {
      time_ns <<= vcpu.tsc_shift;
    } else {
      time_ns >>= -vcpu.tsc_shift;
    }
    time_ns = (time_ns * vcpu.tsc_to_system_mul) >> 32;
    time_ns += vcpu.system_time;
    asm("mfence" ::: "memory");
  }
  while ((vcpu.version & 0x1) || (version != vcpu.version));

  return time_ns;
}

uint64_t KVM_clock::wall_clock()
{
  uint32_t version = 0;
  uint64_t tval = 0;
	do
  {
		version = kvm_wall_clock.version;
		asm("mfence" ::: "memory");
		tval = kvm_wall_clock.sec * 1000000000ull;
		tval += kvm_wall_clock.nsec;
		asm("mfence" ::: "memory");
	}
  while ((kvm_wall_clock.version & 1)
      || (kvm_wall_clock.version != version));

  return tval;
}
