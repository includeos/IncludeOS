// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <profile>
#include <common>
#include <hw/pit.hpp>
#include <kernel/elf.hpp>
#include <kernel/irq_manager.hpp>
#include <util/fixedvec.hpp>
#include <unordered_map>
#include <cassert>
#include <algorithm>

#define BUFFER_COUNT    1024

extern "C" {
  void parasite_interrupt_handler();
  void profiler_stack_sampler(void*);
  void gather_stack_sampling();
}
extern char _irq_cb_return_location;

typedef uint32_t func_sample;
struct Sampler
{
  fixedvector<uintptr_t, BUFFER_COUNT>* samplerq;
  fixedvector<uintptr_t, BUFFER_COUNT>* transferq;
  std::unordered_map<uintptr_t, func_sample> dict;
  uint64_t total  = 0;
  uint64_t asleep = 0;
  int  lockless;
  bool discard; // discard results as long as true

  Sampler() {
    // make room for these only when requested
    #define blargh(T) std::remove_pointer<decltype(T)>::type;
    samplerq = new blargh(samplerq);
    transferq = new blargh(transferq);
    total    = 0;
    asleep   = 0;
    lockless = 0;
    discard  = false;
  }

  void begin() {
    // gather samples repeatedly over small periods
    using namespace std::chrono;
    static const milliseconds GATHER_PERIOD_MS = 150ms;

    hw::PIT::instance().on_repeated_timeout(
        GATHER_PERIOD_MS, gather_stack_sampling);
  }
  void add(void* ra)
  {
    // need free space to take more samples
    if (samplerq->free_capacity())
        samplerq->add((uintptr_t) ra);

    // return when its not our turn
    if (lockless) return;

    // transfer all the built up samplings
    transferq->copy(samplerq->first(), samplerq->size());
    samplerq->clear();
    lockless = 1;
  }
};

Sampler& get() {
  static Sampler sampler;
  return sampler;
}

void StackSampler::begin()
{
  // install interrupt handler
  IRQ_manager::get().set_irq_handler(0, parasite_interrupt_handler);
  // start taking samples using PIT interrupts
  get().begin();
}

void profiler_stack_sampler(void* esp)
{
  void* ra = esp; //__builtin_return_address(1);
  // maybe qemu, maybe some bullshit we don't care about
  if (UNLIKELY(ra == nullptr || get().discard)) return;
  // ignore event loop (and take sleep statistic)
  if (ra == &_irq_cb_return_location) {
    ++get().asleep;
    return;
  }
  // add address to sampler queue
  get().add(ra);
}

void gather_stack_sampling()
{
  // gather results on our turn only
  if (get().lockless == 1)
  {
    for (auto* addr = get().transferq->first(); addr < get().transferq->end(); addr++)
    {
      // convert return address to function entry address
      uintptr_t resolved = Elf::resolve_addr(*addr);
      // insert into unordered map
      auto it = get().dict.find(resolved);
      if (it != get().dict.end()) {
        it->second++;
      }
      else {
        // add to dictionary
        get().dict.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(resolved),
            std::forward_as_tuple(1));
      }
    }
    // increase total and switch back transferring of samples
    get().total += get().transferq->size();
    get().lockless = 0;
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
  printf("Heap begin  %#x  size %u Kb\n",     heap_begin, heap_size / 1024);
  printf("Heap end    %#x  diff %u (%d Kb)\n", heap_end,  last, last / 1024);
  last = (int32_t) heap_size;
}

uint64_t StackSampler::samples_total()
{
  return get().total;
}
uint64_t StackSampler::samples_asleep()
{
  return get().asleep;
}

std::vector<Sample> StackSampler::results(int N)
{
  using sample_pair = std::pair<uintptr_t, func_sample>;
  std::vector<sample_pair> vec(get().dict.begin(), get().dict.end());

  // sort by count
  std::sort(vec.begin(), vec.end(),
  [] (const sample_pair& sample1, const sample_pair& sample2) -> int {
    return sample1.second > sample2.second;
  });

  std::vector<Sample> res;

  N = (N > (int)vec.size()) ? vec.size() : N;
  if (N <= 0) return res;

  for (auto& sa : vec)
  {
    // resolve the addr
    auto func = Elf::resolve_symbol(sa.first);
    res.push_back(Sample {sa.second, (void*) func.addr, func.name});

    if (--N == 0) break;
  }
  return res;
}

void StackSampler::set_mask(bool mask)
{
  get().discard = mask;
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

  func = Elf::resolve_symbol((void*) &StackSampler::set_mask);
  if (func.name != "StackSampler::set_mask(bool)")
      __panic_failure(where, id);
}
