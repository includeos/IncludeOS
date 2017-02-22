#include <hw/apic_revenant.hpp>

#include <hw/apic.hpp>
#include <kernel/irq_manager.hpp>
#include <kprint>

smp_stuff smp;
idt_loc   smp_lapic_idt;

// expensive, but correctly returns the current CPU id
extern "C" void      initialize_cpu_id(int);
extern "C" int       get_cpu_id();
extern "C" uintptr_t get_cpu_esp();
extern "C" void      lapic_exception_handler();
#define INFO(FROM, TEXT, ...) printf("%13s ] " TEXT "\n", "[ " FROM, ##__VA_ARGS__)

struct per_cpu_test
{
  int value;
  int padding[15];
} __attribute__((aligned(64)));
static std::array<per_cpu_test, 16> testing;

template <typename T, size_t N>
inline T& per_cpu(std::array<T, N>& array)
{
  unsigned cpuid;
  asm volatile("movl %%gs, %0" : "=m" (cpuid));
  return array.at(cpuid);
}
#define PER_CPU(x) (per_cpu<decltype(x)::value_type, x.size()>(x))

__attribute__((constructor))
static void init_test()
{
  for (size_t i = 0; i < testing.size(); i++)
    testing[i].value = i;
  
}

void revenant_main(int cpu)
{
  // load IDT
  asm volatile("lidt %0" : : "m"(smp_lapic_idt));
  // enable Local APIC
  hw::APIC::get().smp_enable();
  // initialize cpuid for this core
  initialize_cpu_id(cpu);
  lock(smp.glock);
  kprintf("AP %d started\n", cpu);
  unlock(smp.glock);
  // we can use shared memory here because the
  // bootstrap CPU is waiting on revenants to start
  // we do, however, need to synchronize in between CPUs
  lock(smp.glock);
  INFO("REV", "AP %d started at %#x", get_cpu_id(), get_cpu_esp());
  unlock(smp.glock);
  
  // enable interrupts
  asm volatile("sti");
  
  // signal that the revenant has started
  smp.boot_barrier.inc();
  // sleep
  asm volatile("hlt");
  
  while (true)
  {
    // grab hold on task list
    lock(smp.tlock);
    
    if (smp.tasks.empty()) {
      unlock(smp.tlock);
      // sleep
      asm volatile("hlt");
      // try again
      continue;
    }
    
    // get copy of shared task
    auto task = smp.tasks.front();
    smp.tasks.pop_front();
    
    bool empty = smp.tasks.empty();
    
    unlock(smp.tlock);
    
    // execute actual task
    task.func();
    
    // add done function to completed list (only if its callable)
    lock(smp.flock);
    smp.completed.push_back(task.done);
    unlock(smp.flock);
    
    // at least one thread will empty the task list
    if (empty)
      hw::APIC::get().send_bsp_intr();
  }
  __builtin_unreachable();
}

extern void print_backtrace();

extern "C" {
  extern void (*current_eoi_mechanism)();
  void lapic_irq_handler()
  {
    hw::APIC::get().eoi();
    //(*current_eoi_mechanism)();
  }
  void lapic_except_handler()
  {
    lock(smp.glock);
    /// Oops!
    kprintf(">>> LAPIC %d: Oops! CPU Exception!\n", get_cpu_id());
    /// Backtrace
    print_backtrace();
    unlock(smp.glock);
  }
}
