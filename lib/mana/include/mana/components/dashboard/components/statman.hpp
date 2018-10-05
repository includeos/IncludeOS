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
#ifndef DASHBOARD_COMPONENTS_STATMAN_HPP
#define DASHBOARD_COMPONENTS_STATMAN_HPP

#include "../component.hpp"

#include <statman>

namespace mana {
namespace dashboard {

class Statman : public Component {

public:

  Statman(::Statman& statman)
   : statman_{statman}
  {}

  std::string key() const override
  { return "statman"; }

  void serialize(Writer& writer) override {
    writer.StartArray();
    for(auto it = statman_.begin(); it != statman_.end(); ++it) {
      auto& stat = *it;
      writer.StartObject();

      writer.Key("name");
      writer.String(stat.name());

      writer.Key("value");
      const std::string type = [&](){
        switch(stat.type()) {
          case Stat::UINT64:  writer.Uint64(stat.get_uint64());
                              return "UINT64";
          case Stat::UINT32:  writer.Uint(stat.get_uint32());
                              return "UINT32";
          case Stat::FLOAT:   writer.Double(stat.get_float());
                              return "FLOAT";
        }
      }();

      writer.Key("type");
      writer.String(type);

      writer.Key("index");
      writer.Int(std::distance(statman_.begin(), it));

      writer.EndObject();
    }

    writer.EndArray();
  }

private:
  ::Statman& statman_;

};

} // < namespace dashboard
} // < namespace mana

#endif
