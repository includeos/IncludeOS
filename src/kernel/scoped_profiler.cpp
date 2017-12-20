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
#include <kernel/cpuid.hpp>
#include <kernel/elf.hpp>
#include <kernel/os.hpp>
#include <unordered_map>
#include <cassert>
#include <algorithm>
#include <sstream>

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
                  : "=A" (tick_start));
  }
  else if (guard == Guard::MFENCE)
  {
    asm volatile ("mfence\n\t"
                  "rdtsc\n\t"
                  : "=A" (tick_start));
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
                  : "=A" (tick));
  }
  else if (guard == Guard::MFENCE)
  {
    asm volatile ("mfence\n\t"
                  "rdtsc\n\t"
                  : "=A" (tick));
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
