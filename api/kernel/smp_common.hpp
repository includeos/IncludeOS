#pragma once
#include <smp>
#include <cstdint>
#include <deque>
#include <membitmap>
#include <vector>

namespace smp
{
	struct task {
	  task(SMP::task_func a,
	           SMP::done_func b)
	   : func(a), done(b) {}

	  SMP::task_func func;
	  SMP::done_func done;
	};

	void task_done_handler();
	void smp_task_handler();

	struct smp_main_system
	{
	  uintptr_t stack_base;
	  uintptr_t stack_size;
	  smp_barrier boot_barrier;
	  std::vector<int> initialized_cpus {0};
	  // used to determine which worker has posted done-tasks
	  std::array<uint32_t, 8> bmp_storage = {0};
	  MemBitmap bitmap{bmp_storage.data(), bmp_storage.size()};
	};
	extern smp_main_system main_system;

	struct smp_worker_system
	{
	  smp_spinlock tlock;
	  smp_spinlock flock;
	  std::deque<smp::task> tasks;
	  std::vector<SMP::done_func> completed;
	  bool work_done = false;
	  // main thread on this vCPU
	  long main_thread_id = 0;
	};
	extern std::vector<smp_worker_system> systems;
}
