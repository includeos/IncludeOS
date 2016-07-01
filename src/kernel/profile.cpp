#include <profile>

#include <hw/pit.hpp>
#include <kernel/elf.hpp>
#include <kernel/irq_manager.hpp>
#include <deque>
#include <unordered_map>
#include <cassert>
#include <algorithm>

#define BUFFER_COUNT    6000

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
static fixedvector<uintptr_t, BUFFER_COUNT>* sampler_queue;
static fixedvector<uintptr_t, BUFFER_COUNT>* sampler_transfer;
static void* event_loop_addr;

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
  void gather_stack_sampling();
}

void begin_stack_sampling(uint16_t gather_period_ms)
{
  // make room for these only when requested
  #define blargh(T) std::remove_pointer<decltype(T)>::type;
  sampler_queue = new blargh(sampler_queue);
  sampler_transfer = new blargh(sampler_transfer);
  
  // we want to ignore event loop at FIXME the HLT location (0x198)
  event_loop_addr = (void*) ((char*) &OS::event_loop + 0x198);
  
  // begin sampling
  IRQ_manager::cpu(0).set_irq_handler(0, parasite_interrupt_handler);
  
  // gather every second
  using namespace std::chrono;
  hw::PIT::instance().on_repeated_timeout(milliseconds(gather_period_ms),
  [] { gather_stack_sampling(); });
}

void profiler_stack_sampler()
{
  void* ra = __builtin_return_address(1);
  // maybe qemu, maybe some bullshit we don't care about
  if (ra == nullptr) return;
  // ignore event loop
  if (ra == event_loop_addr) return;
  
  // add to queue
  sampler_queue->add((uintptr_t) ra);
  
  // return when its not our turn
  if (lockless_sampler) return;
  
  // transfer all the built up samplings
  sampler_transfer->clone(sampler_queue->first(), sampler_queue->size());
  sampler_queue->clear();
  lockless_sampler = 1;
}

void gather_stack_sampling()
{
  // gather results on our turn only
  if (lockless_sampler == 1)
  {
    for (auto* addr = sampler_transfer->first(); addr < sampler_transfer->end(); addr++)
    {
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
}

void print_heap_info()
{
  uintptr_t start_addr = Elf::resolve_name("_start");
  printf("start_addr: %#x\n", start_addr);
  
  // also show information on heap end in case of leaks
  extern char* heap_end;
  extern char* heap_begin;
  auto heap_size = (size_t) (heap_end - heap_begin);
  printf("[!] Heap begin  %p Heap size %#x\n",     heap_begin, heap_size);
  printf("[!] Heap end    %p           (%u Kb)\n", heap_end,   heap_size / 1024);
}

void print_stack_sampling()
{
  // sort by count
  using sample_pair = std::pair<uintptr_t, func_sample>;
  std::vector<sample_pair> vec(sampler_dict.begin(), sampler_dict.end());
  std::sort(vec.begin(), vec.end(), 
  [] (const sample_pair& sample1, const sample_pair& sample2) -> int {
    return sample1.second.count > sample2.second.count;
  });
  
  size_t results = 12;
  results = (results > sampler_dict.size()) ? sampler_dict.size() : results;
  
  printf("*** Listing %d samples ***\n", results);
  for (auto& p : vec)
  {
    // resolve the addr
    auto func = Elf::resolve_symbol(p.first);
    // print some shits
    printf("[%#.6x + %#.3x] %u times: %s\n",
        func.addr, func.offset, p.second.count, func.name.c_str());
    
    if (results-- == 0) break;
  }
  print_heap_info();
  printf("*** ---------------------- ***\n");
}


void __panic_failure(char const* where, size_t id)
{
  printf("\n[FAILURE] %s, id=%u\n", where, id);
  print_heap_info();
  while (true)
    asm volatile("cli; hlt");
}

void __validate_backtrace(char const* where, size_t id)
{
  func_offset func;
  
  func = Elf::resolve_symbol((void*) &__validate_backtrace);
  if (func.name != "__validate_backtrace")
      __panic_failure(where, id);
  
  func = Elf::resolve_symbol((void*) &print_stack_sampling);
  if (func.name != "print_stack_sampling()")
      __panic_failure(where, id);
}
