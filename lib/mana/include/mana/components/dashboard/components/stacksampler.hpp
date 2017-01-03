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
#ifndef DASHBOARD_COMPONENTS_STACKSAMPLER_HPP
#define DASHBOARD_COMPONENTS_STACKSAMPLER_HPP

#include "../component.hpp"

#include <profile>

namespace mana {
namespace dashboard {

class StackSampler : public Component {

public:

  static std::shared_ptr<StackSampler> instance() {
    static std::weak_ptr<StackSampler> instance_;
    if(auto p = instance_.lock())
      return p;

    std::shared_ptr<StackSampler> p{new StackSampler};
    instance_ = p;
    return p;
  }

  std::string key() const override
  { return "stack_sampler"; }

  void serialize(Writer& writer) override {

    writer.StartObject();

    auto samples = ::StackSampler::results(sample_size_);
    auto total = ::StackSampler::samples_total();
    auto asleep = ::StackSampler::samples_asleep();

    writer.Key("active");
    double active = total / (double)(total+asleep) * 100.0;
    writer.Double(active);

    writer.Key("asleep");
    double asleep_perc = asleep / (double)(total+asleep) * 100.0;
    writer.Double(asleep_perc);

    writer.Key("samples");
    writer.StartArray();
    for (auto& sa : samples)
    {
      writer.StartObject();

      writer.Key("address");
      writer.Uint((uintptr_t)sa.addr);

      writer.Key("name");
      writer.String(sa.name);

      writer.Key("total");
      writer.Uint(sa.samp);

      // percentage of total samples
      float perc = sa.samp / (float)total * 100.0f;

      writer.Key("percent");
      writer.Double(perc);

      writer.EndObject();
    }
    writer.EndArray();

    writer.EndObject();
  }

  void set_sample_size(int N)
  { sample_size_ = N; }

private:

  StackSampler()
  : sample_size_{12}
  {
    ::StackSampler::begin();
  }

  int sample_size_;

};

} // < namespace dashboard
} // < namespace mana

#endif




