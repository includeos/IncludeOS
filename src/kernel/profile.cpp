#include <profile>

#include <hw/pit.hpp>
#include <kernel/elf.hpp>
#include <kernel/irq_manager.hpp>
#include <unordered_map>
#include <cassert>
#include <algorithm>

#define BUFFER_COUNT    2048

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
  
  bool free_capacity() const noexcept {
    return count < N;
  }
  
  void clone(T* src, uint32_t size) {
    memcpy(element, src, size * sizeof(T));
    count = size;
  }
  
  uint32_t count = 0;
  T element[N];
};
static fixedvector<uintptr_t, BUFFER_COUNT>* sampler_queue;
static fixedvector<uintptr_t, BUFFER_COUNT>* transfer_queue;
static void* event_loop_addr;

typedef uint32_t func_sample;
std::unordered_map<uintptr_t, func_sample> sampler_dict;
static func_sample sampler_total = 0;
static int lockless_sampler = 0;

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
  transfer_queue = new blargh(transfer_queue);
  sampler_total = 0;
  
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
  
  // need free space to take more samples
  if (sampler_queue->free_capacity())
      sampler_queue->add((uintptr_t) ra);
  
  // return when its not our turn
  if (lockless_sampler) return;
  
  // transfer all the built up samplings
  transfer_queue->clone(sampler_queue->first(), sampler_queue->size());
  sampler_queue->clear();
  lockless_sampler = 1;
}

void gather_stack_sampling()
{
  // gather results on our turn only
  if (lockless_sampler == 1)
  {
    for (auto* addr = transfer_queue->first(); addr < transfer_queue->end(); addr++)
    {
      // convert return address to function entry address
      uintptr_t resolved = Elf::resolve_addr(*addr);
      // insert into unordered map
      auto it = sampler_dict.find(resolved);
      if (it != sampler_dict.end()) {
        it->second++;
      }
      else {
        // add to dictionary
        sampler_dict.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(resolved),
            std::forward_as_tuple(1));
      }
    }
    sampler_total += transfer_queue->size();
    lockless_sampler = 0;
  }
}

void print_heap_info()
{
  static intptr_t last = 0;
  // show information on heap status, to discover leaks etc.
  extern uintptr_t heap_begin;
  extern uintptr_t heap_end;
  intptr_t heap_size = heap_end - heap_begin;
  last = heap_size - last;
  printf("[!] Heap information:\n");
  printf("[!] begin  %#x  size %#x (%u Kb)\n", heap_begin, heap_size, heap_size / 1024);
  printf("[!] end    %#x  diff %#x (%d Kb)\n", heap_end,   last, last / 1024);
  last = (int32_t) heap_size;
}

void print_stack_sampling()
{
  using sample_pair = std::pair<uintptr_t, func_sample>;
  
  // sort by count
  std::vector<sample_pair> vec(sampler_dict.begin(), sampler_dict.end());
  std::sort(vec.begin(), vec.end(), 
  [] (const sample_pair& sample1, const sample_pair& sample2) -> int {
    return sample1.second > sample2.second;
  });
  
  size_t results = 12;
  results = (results > sampler_dict.size()) ? sampler_dict.size() : results;
  
  printf("*** Listing %d results (%u samples) ***\n", results, sampler_total);
  for (auto& sa : vec)
  {
    // resolve the addr
    auto func = Elf::resolve_symbol(sa.first);
    
    float f =  (float) sa.second / sampler_total * 100;
    // print some shits
    printf("%5.2f%%  %*u: %s\n",
        f, 8, sa.second, func.name.c_str());
    
    if (results-- == 0) break;
  }
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
