#include <kernel/smp_common.hpp>

namespace smp
{
smp_main_system main_system;
std::vector<smp_worker_system> systems;

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

}
