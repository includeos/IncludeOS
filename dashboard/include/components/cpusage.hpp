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

#include <kernel/irq_manager.hpp>
#include <timers>
#include <delegate>

namespace dashboard {

class CPUsage : public Component {

public:

  CPUsage(::IRQ_manager& manager, Timers::duration_t when, Timers::duration_t interval)
   :  manager_{manager},
      interval_{interval}
  {
    timer_id_ = Timers::periodic(when, interval, [&] (Timers::id_t) {
      old_halt_ = new_halt_;
      old_total_ = new_total_;
      new_halt_ = manager_.cycles_hlt();
      new_total_ = manager_.cycles_total();
    });
  }

  ~CPUsage() {
    Timers::stop(timer_id_);
  }

  std::string key() const override
  { return "cpu_usage"; }

  void serialize(Writer& writer) const override {
    writer.StartObject();

    writer.Key("halt");
    uint64_t halt = new_halt_ - old_halt_;
    writer.Uint64(halt);

    writer.Key("total");
    uint64_t total = new_total_ - old_total_;
    writer.Uint64(total);

    writer.Key("interval");
    writer.Double(interval_.count());

    writer.EndObject();
  }

private:
  uint64_t old_halt_ = 0;
  uint64_t new_halt_ = 0;
  uint64_t old_total_ = 0;
  uint64_t new_total_ = 0;

  ::IRQ_manager& manager_;
  Timers::duration_t interval_;
  Timers::id_t timer_id_;
};

} // < namespace dashboard

#endif
