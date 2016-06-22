#include <profile>

#include <cstdint>
#include <hw/pit.hpp>
#include <kernel/elf.hpp>
#include <kernel/irq_manager.hpp>
#include <deque>
#include <unordered_map>
#include <cassert>
#include <algorithm>

#define BUFFER_COUNT    16000

template <typename T, int N>
struct fixedvector {
  
  void add(const T& e) noexcept {
    assert(count < N);
    element[count] = e;
    count++;
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
fixedvector<uintptr_t, BUFFER_COUNT>* sampler_queue;
fixedvector<uintptr_t, BUFFER_COUNT>* sampler_transfer;

struct func_sample
{
  func_sample(uint32_t c)
    : count(c) {}
  
  uint32_t  count = 0;
};
std::unordered_map<uintptr_t, func_sample> sampler_dict;
static volatile int lockless_sampler = 0;

extern "C" {
  void parasite_interrupt_handler();
  void profiler_stack_sampler();
}

void begin_stack_sampling()
{
  // make room for these only when requested
  #define blargh(T) std::remove_pointer<decltype(T)>::type;
  sampler_queue = new blargh(sampler_queue);
  sampler_transfer = new blargh(sampler_transfer);
  // begin sampling
  printf("Stack sampler taking over PIT\n");
  IRQ_manager::cpu(0).set_irq_handler(0, parasite_interrupt_handler);
}

void profiler_stack_sampler()
{
  void* ra = __builtin_return_address(2);
  // maybe qemu, maybe some bullshit we don't care about
  if (ra == nullptr) return;
  
  // add to queue
  sampler_queue->add((uintptr_t) ra);
  
  // return when its not our turn
  if (lockless_sampler) return;
  
  // transfer all the built up samplings
  sampler_transfer->clone(sampler_queue->first(), sampler_queue->size());
  sampler_queue->clear();
  lockless_sampler = 1;
}


void print_stack_sampling(int results)
{
  // gather results
  if (lockless_sampler == 1)
  {
    for (auto* addr = sampler_transfer->first(); addr < sampler_transfer->end(); addr++)
    {
      assert(*addr);
      auto it = sampler_dict.find(*addr);
      
      if (it != sampler_dict.end()) {
        it->second.count++;
      }
      else {
        // add to dictionary
        sampler_dict.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(*addr),
            std::forward_as_tuple(1));
      }
    }
    lockless_sampler = 0;
  }
  
  // sort by count
  using sample_pair = std::pair<uintptr_t, func_sample>;
  std::vector<sample_pair> vec(sampler_dict.begin(), sampler_dict.end());
  std::sort(vec.begin(), vec.end(), 
  [] (const sample_pair& sample1, const sample_pair& sample2) -> int {
    return sample1.second.count > sample2.second.count;
  });
  
  printf("*** Listing %d/%u samples ***\n", results, sampler_dict.size());
  for (auto& p : vec)
  {
    // resolve the addr
    auto func = Elf::resolve_symbol(p.first);
    // print some shits
    printf("[%#x + %#x] %u times: %s\n",
        func.addr, func.offset, p.second.count, func.name.c_str());
    
    if (--results <= 0) break;
  }
  printf("*** ---------------------- ***\n");
}
