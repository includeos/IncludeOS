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

namespace mana {
namespace dashboard {

class CPUsage : public Component {

public:

  CPUsage(Timers::duration_t when, Timers::duration_t interval)
   :  interval_{interval},
      timer_id_{Timers::periodic(when, interval, {this, &CPUsage::update_values})}
  {}

  ~CPUsage() {
    Timers::stop(timer_id_);
  }

  std::string key() const override
  { return "cpu_usage"; }

  void serialize(Writer& writer) override {
    writer.StartObject();

    writer.Key("halt");

    if(new_halt_ > old_halt_)
      serialized_halt_ = new_halt_ - old_halt_;

    writer.Uint64(serialized_halt_);

    writer.Key("total");

    if(new_total_ > old_total_)
      serialized_total_ = new_total_ - old_total_;

    writer.Uint64(serialized_total_);

    writer.Key("interval");
    writer.Double(interval_.count());

    writer.EndObject();
  }

private:
  uint64_t old_halt_ = 0;
  uint64_t new_halt_ = 0;
  uint64_t old_total_ = 0;
  uint64_t new_total_ = 0;

  uint64_t serialized_halt_ = 0;
  uint64_t serialized_total_ = 0;

  Timers::duration_t interval_;
  Timers::id_t timer_id_;

  void update_values(Timers::id_t) {
    uint64_t temp_halt = new_halt_;
    uint64_t temp_total = new_total_;

    new_halt_ = OS::get_cycles_halt();
    new_total_ = OS::get_cycles_total();
    old_halt_ = temp_halt;
    old_total_ = temp_total;
  }
};

} // < namespace dashboard
} // < namespace mana

#endif
