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

//#define DEBUG
#include <hw/cpu_freq_sampling.hpp>
#include <kernel/irq_manager.hpp>
#include <kernel/os.hpp>
#include <vector>
#include <algorithm>

extern "C"
const uint16_t _cpu_sampling_freq_divider_ = KHz(hw::PIT::frequency()).count() * 10; // Run 1 KHz  Lowest: 0xffff

namespace hw {

  static std::vector<uint64_t> _cpu_timestamps;

  constexpr static MHz test_frequency() {
    return MHz(PIT::frequency().count() / _cpu_sampling_freq_divider_);
  }
  
  double calculate_cpu_frequency(size_t samples)
  {
    // We expect the cpu_sampling_irq_handler to push in samples;
    while (_cpu_timestamps.size() < samples)
        OS::halt();

    // Subtract the time it takes to measure time :-)
    auto t1 = OS::cycles_since_boot();
    OS::cycles_since_boot();
    auto t3 = OS::cycles_since_boot();
    auto overhead = (t3 - t1) * 2;

    std::vector<double> cpu_freq_samples;

    for (size_t i = 1; i < _cpu_timestamps.size(); i++){
      // Compute delta in cycles
      auto cycles = _cpu_timestamps[i] - _cpu_timestamps[i-1] + overhead;
      // Cycles pr. second == Hertz
      auto freq = cycles * test_frequency().count();
      cpu_freq_samples.push_back(freq);    
    }

    std::sort(cpu_freq_samples.begin(), cpu_freq_samples.end());
    return cpu_freq_samples[cpu_freq_samples.size() / 2];
  }

} //< namespace hw

extern "C"
void cpu_sampling_irq_handler()
{
  hw::_cpu_timestamps.push_back(OS::cycles_since_boot());
}
