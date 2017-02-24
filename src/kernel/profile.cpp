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
#include "../arch/x86/pit.hpp"
#include <kernel/cpuid.hpp>
#include <kernel/elf.hpp>
#include <kernel/irq_manager.hpp>
#include <kernel/os.hpp>
#include <util/fixedvec.hpp>
#include <unordered_map>
#include <cassert>
#include <algorithm>
#include <sstream>

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
  fixedvector<uintptr_t, BUFFER_COUNT>* samplerq;
  fixedvector<uintptr_t, BUFFER_COUNT>* transferq;
  std::unordered_map<uintptr_t, func_sample> dict;
  uint64_t total  = 0;
  uint64_t asleep = 0;
  int  lockless;
  bool discard; // discard results as long as true
  StackSampler::mode_t mode = StackSampler::MODE_CURRENT;

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

    x86::PIT::instance().on_repeated_timeout(
        GATHER_PERIOD_MS, gather_stack_sampling);
  }
  void add(void* current, void* ra)
  {
    // need free space to take more samples
    if (samplerq->free_capacity()) {
      if (mode == StackSampler::MODE_CURRENT)
          samplerq->add((uintptr_t) current);
      else
          samplerq->add((uintptr_t) ra);
    }
    // return when its not our turn
    if (lockless) return;

    // transfer all the built up samplings
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
  // install interrupt handler
  IRQ_manager::get().set_irq_handler(0, parasite_interrupt_handler);
  // start taking samples using PIT interrupts
  get().begin();
}
void StackSampler::set_mode(mode_t md)
{
  get().mode = md;
}

void profiler_stack_sampler(void* esp)
{
  void* current = esp; //__builtin_return_address(1);
  // maybe qemu, maybe some bullshit we don't care about
  if (UNLIKELY(current == nullptr || get().discard)) return;
  // ignore event loop (and take sleep statistic)
  if (current == &_irq_cb_return_location) {
    ++get().asleep;
    return;
  }
  // add address to sampler queue
  get().add(current, __builtin_return_address(1));
}

static void gather_stack_sampling()
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
    // increase total and switch back transferring of samples
    get().total += get().transferq->size();
    get().lockless = 0;
  }
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
      int len = snprintf(buffer, sizeof(buffer), "0x%08x", func.addr);
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

  printf("Stack sampling - %d results (%u samples)\n",
         samp.size(), total);
  for (auto& sa : samp)
  {
    // percentage of total samples
    float perc = sa.samp / (float)total * 100.0f;
    printf("%5.2f%%  %*u: %.*s\n",
           perc, 8, sa.samp, sa.name.size(), sa.name.c_str());
  }
}

void StackSampler::set_mask(bool mask)
{
  get().discard = mask;
}

decltype(ScopedProfiler::guard)   ScopedProfiler::guard   = Guard::NOT_SELECTED;
decltype(ScopedProfiler::entries) ScopedProfiler::entries = {};

void ScopedProfiler::record()
{
  // Select which guard to use (this is only done once)
  if (UNLIKELY(guard == Guard::NOT_SELECTED))
  {
    if (CPUID::is_intel_cpu() && CPUID::has_feature(CPUID::Feature::SSE2))
    {
      debug2("ScopedProfiler selected guard LFENCE\n");
      guard = Guard::LFENCE;
    }
    else if (CPUID::is_amd_cpu() && CPUID::has_feature(CPUID::Feature::SSE2))
    {
      debug2("ScopedProfiler selected guard MFENCE\n");
      guard = Guard::MFENCE;
    }
    else
    {
      printf("[WARNING] ScopedProfiler only works with an Intel or AMD CPU that supports SSE2!\n");
      guard = Guard::NOT_AVAILABLE;
    }
  }

  if (UNLIKELY(guard == Guard::NOT_AVAILABLE))
  {
    return;  // No guard available -> just bail out
  }
  else if (guard == Guard::LFENCE)
  {
    asm volatile ("lfence\n\t"
                  "rdtsc\n\t"
                  : "=A" (tick_start)
                  :: "%eax", "%ebx", "%ecx", "%edx");
  }
  else if (guard == Guard::MFENCE)
  {
    asm volatile ("mfence\n\t"
                  "rdtsc\n\t"
                  : "=A" (tick_start)
                  :: "%eax", "%ebx", "%ecx", "%edx");
  }
}

ScopedProfiler::~ScopedProfiler()
{
  uint64_t tick = 0;

  if (guard == Guard::NOT_AVAILABLE)
  {
    return;  // No guard available -> just bail out
  }
  else if (guard == Guard::LFENCE)
  {
    asm volatile ("lfence\n\t"
                  "rdtsc\n\t"
                  : "=A" (tick)
                  :: "%eax", "%ebx", "%ecx", "%edx");
  }
  else if (guard == Guard::MFENCE)
  {
    asm volatile ("mfence\n\t"
                  "rdtsc\n\t"
                  : "=A" (tick)
                  :: "%eax", "%ebx", "%ecx", "%edx");
  }

  auto cycles = tick - tick_start;
  auto function_address = __builtin_return_address(0);

  // Find an entry that matches this function_address, or an unused entry
  for (auto& entry : entries)
  {
    if (entry.function_address == function_address)
    {
      // Update the entry
      entry.cycles_average = ((entry.cycles_average * entry.num_samples) + cycles) / (entry.num_samples + 1);
      entry.num_samples += 1;

      return;
    }
    else if (entry.function_address == 0)
    {
      // Use this unused entry
      char symbol_buffer[1024];
      const auto symbols = Elf::safe_resolve_symbol(function_address,
                                                    symbol_buffer,
                                                    sizeof(symbol_buffer));
      entry.name = this->name;
      entry.function_address = function_address;
      entry.function_name = symbols.name;
      entry.cycles_average = cycles;
      entry.num_samples = 1;
      return;
    }
  }

  // We didn't find neither an entry for the function nor an unused entry
  // Warn that the array is too small for the current number of ScopedProfilers
  printf("[WARNING] There are too many ScopedProfilers in use\n");
}

std::string ScopedProfiler::get_statistics()
{
  std::ostringstream ss;

  // Add header
  ss << " CPU time (average) | Samples | Function Name \n";
  ss << "--------------------------------------------------------------------------------\n";

  // Calculate the number of used entries
  auto num_entries = 0u;
  for (auto i = 0u; i < entries.size(); i++)
  {
    if (entries[i].function_address == 0)
    {
      num_entries = i;
      break;
    }
  }

  if (num_entries > 0)
  {
    // Sort on cycles_average (higher value first)
    // Make sure to keep unused entries last (only sort used entries)
    std::sort(entries.begin(), entries.begin() + num_entries, [](const Entry& a, const Entry& b)
    {
      return a.cycles_average > b.cycles_average;
    });

    // Add each entry
    ss.setf(std::ios_base::fixed);
    for (auto i = 0u; i < num_entries; i++)
    {
      const auto& entry = entries[i];
      double  div  = OS::cpu_freq().count() * 1000.0;

      ss.width(16);
      ss << entry.cycles_average / div << " ms | ";

      ss.width(7);
      ss << entry.num_samples << " | ";

      ss.width(0);
      ss << entry.function_name;
      // optional name
      if (entry.name)
        ss << " (" << entry.name << ")";
      
      ss << "\n";
    }
  }
  else
  {
    ss << " <No entries> \n";
  }

  // Add footer
  ss << "--------------------------------------------------------------------------------\n";

  return ss.str();
}
