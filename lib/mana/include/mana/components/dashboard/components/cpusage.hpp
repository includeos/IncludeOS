// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

#pragma once
#ifndef DASHBOARD_COMPONENTS_CPUSAGE_HPP
#define DASHBOARD_COMPONENTS_CPUSAGE_HPP

#include "../component.hpp"
#include <timers>
#include <delegate>
#include <profile>

namespace mana {
namespace dashboard {

class CPUsage : public Component {
public:
  CPUsage() = default;
  ~CPUsage() = default;

  std::string key() const override
  { return "cpu_usage"; }

  void serialize(Writer& writer) override
  {
    uint64_t tdiff = ::StackSampler::samples_total() - last_total;
    last_total = ::StackSampler::samples_total();
    uint64_t adiff = ::StackSampler::samples_asleep() - last_asleep;
    last_asleep = ::StackSampler::samples_asleep();

    double asleep = 1.0;
    if (tdiff > 0) asleep = adiff / (double) tdiff;

    writer.StartObject();
    writer.Key("idle");
    writer.Uint64(asleep * 100.0);

    writer.Key("active");
    writer.Uint64((1.0 - asleep) * 100.0);
    writer.EndObject();
  }

private:
  uint64_t last_asleep = 0;
  uint64_t last_total = 0;
};

} // < namespace dashboard
} // < namespace mana

#endif
