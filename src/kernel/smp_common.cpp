#include <kernel/smp_common.hpp>

#include <kernel/threads.hpp>
#include <common>

namespace smp
{
smp_main_system main_system;
std::vector<smp_worker_system> systems;
// the main CPU is always initialized
std::vector<int> initialized_cpus {0};

void task_done_handler()
{
	int next = smp::main_system.bitmap.first_set();
    while (next != -1)
    {
      // remove bit
      smp::main_system.bitmap.atomic_reset(next);
      // get jobs from other CPU
      std::vector<SMP::done_func> done;
	  auto& system = smp::systems[next];
      system.flock.lock();
      system.completed.swap(done);
      system.flock.unlock();

      // execute all tasks
      for (auto& func : done) func();

      // get next set bit
      next = smp::main_system.bitmap.first_set();
    }
}

static bool smp_task_doer(smp_worker_system& system)
{
  // early return check, as there is no point in locking when its empty
  if (system.tasks.empty()) return false;

  // grab hold on task list
  system.tlock.lock();

  if (system.tasks.empty()) {
    system.tlock.unlock();
    // don't try again
    return false;
  }

  // pick up a task
  smp::task task = std::move(system.tasks.front());
  system.tasks.pop_front();

  system.tlock.unlock();

  // execute actual task
  task.func();

  // add done function to completed list (only if its callable)
  if (task.done != nullptr)
  {
    // NOTE: specifically pushing to this cpu here, and not main system
    auto& system = PER_CPU(smp::systems);
    system.flock.lock();
    system.completed.push_back(std::move(task.done));
    system.flock.unlock();
    // signal home
    system.work_done = true;
  }
  return true;
}
void smp_task_handler()
{
  auto& system = PER_CPU(smp::systems);
  system.work_done = false;
  // cpu-specific tasks
  while (smp_task_doer(system));
  // global tasks (by taking from index 0)
  while (smp_task_doer(systems[0]));
  // if we did any work with done functions, signal back
  if (system.work_done) {
    // set bit for this CPU
    smp::main_system.bitmap.atomic_set(SMP::cpu_id());
    // signal main CPU
	SMP::signal_bsp();
  }
}

} // namespace smp

/// implementation of the SMP interface ///

bool SMP::add_task(SMP::task_func task, SMP::done_func done, int cpu)
{
  auto& system = smp::systems.at(cpu);
  system.tlock.lock();
  bool first = system.tasks.empty();
  system.tasks.emplace_back(std::move(task), std::move(done));
  system.tlock.unlock();
  return first;
}
bool SMP::add_task(SMP::task_func task, int cpu)
{
  return add_task(std::move(task), nullptr, cpu);
}

bool SMP::add_bsp_task(SMP::done_func task)
{
  // queue completed job at current CPU
  auto& system = PER_CPU(smp::systems);
  system.flock.lock();
  bool first = system.completed.empty();
  system.completed.push_back(std::move(task));
  system.flock.unlock();
  // set bit for current CPU
  smp::main_system.bitmap.atomic_set(SMP::cpu_id());
  return first;
}

static smp_spinlock g_global_lock;

void SMP::global_lock() noexcept
{
  g_global_lock.lock();
}
void SMP::global_unlock() noexcept
{
  g_global_lock.unlock();
}


using namespace kernel;

inline auto migration_handler(Thread* kthread)
{
	return [kthread] () -> void {
#ifdef THREADS_DEBUG
		THPRINT("CPU %d resuming migrated thread %d (stack=%p)\n",
			  SMP::cpu_id(), kthread->tid,
			  (void*) kthread->my_stack);
#endif
		// attach this thread on this core
		ThreadManager::get().attach(kthread);
		// resume kthread after yielding this thread
		ThreadManager::get().yield_to(kthread);
		// NOTE: returns here!!
	};
}

void SMP::migrate_threads(bool enable)
{
  if (enable) {
	  ThreadManager::get().on_new_thread =
	  [] (ThreadManager&, Thread* kthread) -> Thread* {
		  SMP::add_task(migration_handler(kthread), nullptr);
		  SMP::signal();
		  return nullptr;
	  };
  } else {
      kernel::ThreadManager::get().on_new_thread = nullptr;
  }
}
void SMP::migrate_threads_to(int cpu)
{
  Expects(cpu >= 0);
  if (cpu == SMP::cpu_id()) {
	  // running threads on this CPU doesn't require migration
      kernel::ThreadManager::get().on_new_thread = nullptr;
  } else {
	  ThreadManager::get().on_new_thread =
	  [cpu] (ThreadManager&, Thread* kthread) -> Thread* {
		  if (cpu != 0) {
		  	SMP::add_task(migration_handler(kthread), nullptr, cpu);
		  	SMP::signal(cpu);
		  }
		  else {
			SMP::add_bsp_task(migration_handler(kthread));
  		  	SMP::signal_bsp();
		  }
		  return nullptr;
	  };
  }
}
