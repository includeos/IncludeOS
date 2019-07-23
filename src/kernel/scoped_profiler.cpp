
#include <profile>
#include <common>
#include <kernel/cpuid.hpp>
#include <kernel/elf.hpp>
#include <os.hpp>
#include <kernel/rtc.hpp>
#include <util/units.hpp>
#include <unordered_map>
#include <cassert>
#include <algorithm>
#include <sstream>

decltype(ScopedProfiler::guard)   ScopedProfiler::guard   = Guard::NOT_SELECTED;
decltype(ScopedProfiler::entries) ScopedProfiler::entries;
static uint64_t base_ticks = 0;

void ScopedProfiler::record()
{
  // Select which guard to use (this is only done once)
  if (UNLIKELY(guard == Guard::NOT_SELECTED))
  {
    if (CPUID::is_intel_cpu())
    {
      debug2("ScopedProfiler selected guard LFENCE\n");
      guard = Guard::LFENCE;
    }
    else if (CPUID::is_amd_cpu())
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
  if (base_ticks == 0) base_ticks = tick_start;
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

  const uint64_t cycles = tick - this->tick_start;
  auto function_address = __builtin_return_address(0);

  // Find an entry that matches this function_address
  for (auto& entry : entries)
  {
    if (entry.function_address == function_address)
    {
      // Update the entry
      entry.cycles_total += cycles;
      entry.num_samples += 1;
      return;
    }
  }
  // Find an unused entry
  for (auto& entry : entries)
  {
    if (entry.function_address == 0)
    {
      // Use this unused entry
      char symbol_buffer[8192];
      const auto symbols = Elf::safe_resolve_symbol(function_address,
                                                    symbol_buffer,
                                                    sizeof(symbol_buffer));
      entry.name = this->name;
      entry.function_address = function_address;
      entry.function_name = symbols.name;
	  entry.num_samples = 1;
      entry.cycles_total = cycles;
	  entry.ticks_start = this->tick_start;
      return;
    }
  }

  // We didn't find neither an entry for the function nor an unused entry
  // Warn that the array is too small for the current number of ScopedProfilers
  printf("[WARNING] There are too many ScopedProfilers in use\n");
}

std::string ScopedProfiler::get_statistics(bool sorted)
{
  std::ostringstream ss;

  // Add header
  ss << " First seen   | CPU time (avg) | Samples | Function Name \n";
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
    if (sorted)
    {
      // Sort on cycles_average (higher value first)
      // Make sure to keep unused entries last (only sort used entries)
      std::sort(entries.begin(), entries.begin() + num_entries, [](const Entry& a, const Entry& b)
      {
        return a.cycles_average() < b.cycles_average();
      });
    }

    // Add each entry
    ss.setf(std::ios_base::fixed);
    for (unsigned i = 0; i < num_entries; i++)
    {
      using namespace util;
      const auto& entry = entries[i];

	  const uint64_t tickdiff = entry.ticks_start - base_ticks;
      double timst = ((double) tickdiff / KHz(os::cpu_freq()).count());
      ss.width(10);
      ss << timst << " ms | ";

      double micros = (double) entry.cycles_average() / KHz(os::cpu_freq()).count();
      ss.width(10);
      ss << micros << " ms | ";

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
