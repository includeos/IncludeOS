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
#include "cpu_freq_sampling.hpp"
#include "pit.hpp"
#include <os.hpp>
#include <algorithm>
#include <util/units.hpp>
#include <vector>

using namespace util::literals;

extern "C"
const uint16_t _cpu_sampling_freq_divider_ = KHz(x86::PIT::FREQUENCY).count() * 10; // Run 1 KHz  Lowest: 0xffff

namespace x86 {

  static int64_t cpu_timestamps[CPU_FREQUENCY_SAMPLES];
  static size_t  sample_counter = 0;

  constexpr static MHz test_frequency() {
    return MHz(PIT::FREQUENCY.count() / _cpu_sampling_freq_divider_);
  }

  void reset_cpufreq_sampling()
  {
    sample_counter = 0;
  }

  double calculate_cpu_frequency()
  {
    // We expect the cpu_sampling_irq_handler to push in samples;
    while (sample_counter < CPU_FREQUENCY_SAMPLES)
        os::halt();

    // Subtract the time it takes to measure time :-)
    auto t1 = os::cycles_since_boot();
    os::cycles_since_boot();
    auto t3 = os::cycles_since_boot();
    auto overhead = (t3 - t1) * 2;

    std::vector<double> cpu_freq_samples;

    for (size_t i = 1; i < CPU_FREQUENCY_SAMPLES; i++){
      // Compute delta in cycles
      auto cycles = cpu_timestamps[i] - cpu_timestamps[i-1] + overhead;
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
  volatile uint64_t ts = os::cycles_since_boot();
  /// it's forbidden to use heap inside here
  if (x86::sample_counter < x86::CPU_FREQUENCY_SAMPLES) {
    x86::cpu_timestamps[x86::sample_counter++] = ts;
  }
}
