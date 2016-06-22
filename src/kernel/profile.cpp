#include <profile>

#include <cstdint>
#include <hw/pit.hpp>
#include <kernel/elf.hpp>
#include <kernel/irq_manager.hpp>
#include <deque>
#include <unordered_map>
#include <cassert>

#define BUFFER_COUNT    16000

template <typename T, int N>
struct fixedvector {
  
  void add(const T& e) noexcept {
    assert(count < N);
    element[count] = e;
    count++;
  }
  void add(T* e) noexcept {
    add(*e);
  }
  
  void clear() noexcept {
    count = 0;
  }
  uint32_t size() const noexcept {
    return count;
  }
  
  T* first() noexcept {
    return &element[0];
  }
  T* end() noexcept {
    return &element[count];
  }
  
  void clone(T* src, uint32_t size) {
    assert(size <= N);
    memcpy(element, src, size * sizeof(T));
    count = size;
  }
  
  uint32_t count = 0;
  T element[N];
};
fixedvector<safe_func_offset, BUFFER_COUNT>* sampler_queue;
fixedvector<safe_func_offset, BUFFER_COUNT>* sampler_transfer;

struct func_sample
{
  func_sample(const func_offset& f, uint32_t c)
    : func(f), count(c) {}
  
  func_offset func;
  uint32_t    count = 0;
};
std::unordered_map<uintptr_t, func_sample> sampler_dict;
static volatile int lockless_sampler = 0;

#define SAFE_BUFFER_LENGTH  256
static char* __sampler_safe_buffer;
inline char* get_sampler_safe_buffer(size_t index) {
  return __sampler_safe_buffer + SAFE_BUFFER_LENGTH * index;
}

#define blargh(T) std::remove_pointer<decltype(T)>::type;

extern "C" {
  void parasite_interrupt_handler();
  void profiler_stack_sampler();
}
extern void print_backtrace();

void begin_stack_sampling()
{
  // make room for these only when requested
  __sampler_safe_buffer = new char[SAFE_BUFFER_LENGTH * BUFFER_COUNT];
  sampler_queue = new blargh(sampler_queue);
  sampler_transfer = new blargh(sampler_transfer);
  // begin sampling
  printf("Stack sampler taking over PIT\n");
  IRQ_manager::cpu(0).set_irq_handler(0, parasite_interrupt_handler);
}

void profiler_stack_sampler()
{
  void* ra = __builtin_return_address(2);
  // select heap-safe buffer based on sampler queue index
  static int current = 0;
  char* buffer = get_sampler_safe_buffer(current);
  // heap safe symbol resolver and demangle
  auto safe_func = Elf::resolve_symbol(ra, buffer, SAFE_BUFFER_LENGTH);
  // use another buffer next time
  current = (current + 1) % BUFFER_COUNT;
  
  // add to queue
  sampler_queue->add(safe_func);
  
  if (lockless_sampler == 0) {
    sampler_transfer->clone(sampler_queue->first(), sampler_queue->size());
    sampler_queue->clear();
    lockless_sampler = 1;
  }
}

void print_stack_sampling(int results)
{
  if (lockless_sampler == 1)
  {
    for (auto* func = sampler_transfer->first(); func < sampler_transfer->end(); func++)
    {
      auto it = sampler_dict.find(func->addr);
      if (it != sampler_dict.end())
        (*it).second.count++;
      else {
        // make a proper copy of the name so that the buffers can reuse the buffer
        func_offset newfunc;
        newfunc.name = std::string(func->name);
        newfunc.addr = func->addr;
        newfunc.offset = func->offset;
        // add to dictionary
        sampler_dict.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(newfunc.addr),
            std::forward_as_tuple(newfunc, 1));
      }
    }
    lockless_sampler = 0;
  }
  
  printf("* Listing %d/%u samples\n", results, sampler_dict.size());
  for (auto& p : sampler_dict)
  {
    auto& ent = p.second;
    printf("[%#x + %#x]  %s:  %u times\n",
      ent.func.addr, ent.func.offset, ent.func.name.c_str(), ent.count);
    
    if (--results <= 0) return;
  }
}
