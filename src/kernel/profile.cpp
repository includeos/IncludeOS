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
#include <kernel.hpp>
#include <kernel/cpuid.hpp>
#include <kernel/elf.hpp>
#include <os.hpp>
#include <util/fixed_vector.hpp>
#include <unordered_map>
#include <cassert>
#include <algorithm>

#define BUFFER_COUNT    1024

extern "C" {
  void parasite_interrupt_handler();
  void profiler_stack_sampler(void*);
  static void gather_stack_sampling();
}
extern char _irq_cb_return_location;

typedef uint32_t func_sample;
struct Sampler
{
  Fixed_vector<uintptr_t, BUFFER_COUNT>* samplerq;
  Fixed_vector<uintptr_t, BUFFER_COUNT>* transferq;
  std::unordered_map<uintptr_t, func_sample> dict;
  uint64_t total  = 0;
  uint64_t asleep = 0;
  int  lockless = 0;
  bool discard = false; // discard results as long as true
  StackSampler::mode_t mode = StackSampler::MODE_CURRENT;

  Sampler() {
    // make room for these only when requested
    #define blargh(T) std::remove_pointer<decltype(T)>::type;
    samplerq = new blargh(samplerq);
    transferq = new blargh(transferq);
  }

  void begin() {
    // gather samples repeatedly over single period
    __arch_preempt_forever(gather_stack_sampling);
    // install interrupt handler (NOTE: after "initializing" PIT)
    __arch_install_irq(0, parasite_interrupt_handler);
  }
  void add(void* current)
  {
    // need free space to take more samples
    if (samplerq->free_capacity()) {
      if (mode == StackSampler::MODE_CURRENT)
          samplerq->push_back((uintptr_t) current);
      else if (mode == StackSampler::MODE_CALLER)
          samplerq->push_back((uintptr_t) __builtin_return_address(2));
    }
    // return when its not our turn
    if (lockless) return;

    // transfer all the built up samples
    transferq->copy(samplerq->begin(), samplerq->size());
    samplerq->clear();
    lockless = 1;
  }
};

static Sampler& get() {
  static Sampler sampler;
  return sampler;
}

void StackSampler::begin()
{
  // start taking samples using PIT interrupts
  get().begin();
}
void StackSampler::set_mode(mode_t md)
{
  get().mode = md;
}

void profiler_stack_sampler(void* sample)
{
  auto& system = get();
  if (UNLIKELY(sample == nullptr)) return;
  // gather sample statistics
  system.total++;
  if (sample == &_irq_cb_return_location) {
    system.asleep++;
    return;
  }
  // if discard enabled, ignore samples
  if (UNLIKELY(system.discard)) return;
  // add address to sampler queue
  system.add(sample);
}

void gather_stack_sampling()
{
  // gather results on our turn only
  if (get().lockless == 1)
  {
    for (auto* addr = get().transferq->begin(); addr < get().transferq->end(); addr++)
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
    // switch back transferring of samples
    get().lockless = 0;
  }
}

uint64_t StackSampler::samples_total() noexcept {
  return get().total;
}
uint64_t StackSampler::samples_asleep() noexcept {
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
  char buffer[8192];

  N = (N > (int)vec.size()) ? vec.size() : N;
  if (N <= 0) return res;

  for (auto& sa : vec)
  {
    // resolve the addr
    auto func = Elf::safe_resolve_symbol((void*) sa.first, buffer, sizeof(buffer));
    if (func.name) {
      res.push_back(Sample {sa.second, (void*) func.addr, func.name});
    }
    else {
      int len = snprintf(buffer, sizeof(buffer), "0x%08zx", func.addr);
      res.push_back(Sample {sa.second, (void*) func.addr, std::string(buffer, len)});
    }

    if (--N == 0) break;
  }
  return res;
}

void StackSampler::print(const int N)
{
  auto samp = results(N);
  int total = samples_total();

  printf("Stack sampling - %zu results (%d samples)\n",
         samp.size(), total);
  for (auto& sa : samp)
  {
    // percentage of total samples
    float perc = sa.samp / (float)total * 100.0f;
    printf("%5.2f%%  %*u: %.*s\n",
           perc, 8, sa.samp, (int) sa.name.size(), sa.name.c_str());
  }
}

void StackSampler::set_mask(bool mask)
{
  get().discard = mask;
}

std::string HeapDiag::to_string()
{
  static intptr_t last = 0;
  // show information on heap status, to discover leaks etc.
  auto heap_begin = kernel::heap_begin();
  auto heap_end   = kernel::heap_end();
  auto heap_usage = kernel::heap_usage();
  intptr_t heap_size = heap_end - heap_begin;
  last = heap_size - last;

  char buffer[256];
  int len = snprintf(buffer, sizeof(buffer),
          "Heap begin  %#lx  size %lu Kb\n"
          "Heap end    %#lx  diff %lu (%ld Kb)\n"
          "Heap usage  %lu kB\n",
          heap_begin, heap_size / 1024,
          heap_end,  last, last / 1024,
          heap_usage / 1024);
  last = (int32_t) heap_size;
  return std::string(buffer, len);
}
