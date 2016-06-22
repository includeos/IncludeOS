#include <profile>

#include <cstdint>
#include <hw/pit.hpp>
#include <kernel/elf.hpp>
#include <kernel/irq_manager.hpp>
#include <unordered_map>

struct func_sample
{
  func_offset func;
  uint32_t    count = 0;
};
std::unordered_map<uintptr_t, func_sample> sampler;

extern "C" {
  void parasite_interrupt_handler();
  void profiler_stack_sampler();
}
extern void print_backtrace();

void begin_stack_sampling()
{
  printf("Stack sampler taking over PIT\n");
  IRQ_manager::cpu(0).set_irq_handler(0, parasite_interrupt_handler);
}

void profiler_stack_sampler()
{
  void* ra = __builtin_return_address(2);
  auto func = Elf::resolve_symbol(ra);
  
  auto& sample = sampler[func.addr];
  sample.func = func;
  sample.count++;
}

void print_stack_sampling(int results)
{
  printf("* Listing %u samples\n", sampler.size());
  for (auto& p : sampler)
  {
    auto& ent = p.second;
    printf("[%#x + %#x]  %s:  %u times\n",
      ent.func.addr, ent.func.offset, ent.func.name.c_str(), ent.count);
    
    if (--results <= 0) return;
  }
}
