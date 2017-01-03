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
#ifndef DASHBOARD_COMPONENTS_LOGGER_HPP
#define DASHBOARD_COMPONENTS_LOGGER_HPP

#include "../component.hpp"

#include <util/logger.hpp>

namespace mana {
namespace dashboard {

class Logger : public Component {

public:

  Logger(::Logger& logger, size_t entries = 50)
   : logger_{logger}, entries_{entries}
  {}

  std::string key() const override
  { return "logger"; }

  void serialize(Writer& writer) override {
    writer.StartArray();
    auto entries = (entries_) ? logger_.entries(entries_) : logger_.entries();

    auto it = entries.begin();

    while(it != entries.end())
      writer.String(*it++);

    writer.EndArray();
  }

private:
  const ::Logger& logger_;
  const size_t entries_;

};

} // < namespace dashboard
} // < namespace mana

#endif




