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
#ifndef DASHBOARD_COMPONENTS_STATUS_HPP
#define DASHBOARD_COMPONENTS_STATUS_HPP

#include "../component.hpp"

#include <os>
#include <rtc>

namespace dashboard {

class Status : public Component {

public:

  static Status& instance() {
    static Status s;
    return s;
  }

  std::string key() const override
  { return "status"; }

  void serialize(Writer& writer) const override {
    writer.StartObject();

    writer.Key("version");
    writer.String(OS::version());

    writer.Key("service");
    writer.String(Service::name());

    writer.Key("uptime");
    writer.Int64(OS::uptime());

    writer.Key("heap_usage");
    writer.Uint64(OS::heap_usage());

    writer.Key("cpu_freq");
    writer.Double(OS::cpu_freq().count());

    writer.Key("current_time");
    long hest = RTC::now();
    struct tm* tt =
      gmtime (&hest);
    char datebuf[32];
    strftime(datebuf, sizeof datebuf, "%FT%TZ", tt);
    writer.String(datebuf);

    writer.EndObject();
  }

private:

  Status() {};

};

} // < namespace dashboard

#endif




