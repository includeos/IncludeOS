
#pragma once
#ifndef X86_APIC_REVENANT_HPP
#define X86_APIC_REVENANT_HPP

#include "smp.hpp"
#include <cstdint>
#include <deque>
#include <membitmap>
#include <vector>
#include <thread>

extern "C" void revenant_main(int);
extern void revenant_thread_main(int cpu);

namespace x86 {
struct smp_task {
  smp_task(SMP::task_func a,
           SMP::done_func b)
   : func(a), done(b) {}

  SMP::task_func func;
  SMP::done_func done;
};

struct smp_stuff
{
  uintptr_t stack_base;
  uintptr_t stack_size;
  minimal_barrier_t boot_barrier;
  std::vector<int> initialized_cpus {0};
  std::array<uint32_t, 8> bmp_storage = {0};
  MemBitmap bitmap{bmp_storage.data(), bmp_storage.size()};
};
extern smp_stuff smp_main;

struct smp_system_stuff
{
  smp_spinlock tlock;
  smp_spinlock flock;
  std::vector<smp_task> tasks;
  std::vector<SMP::done_func> completed;
  bool work_done = false;
  // main thread on this vCPU
  std::thread* main_thread = nullptr;
  long         main_thread_id = 0;
};
 extern SMP::Array<smp_system_stuff> smp_system;
}

#endif
