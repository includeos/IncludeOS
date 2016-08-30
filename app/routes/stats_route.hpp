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

#ifndef ROUTES_STATS_ROUTE_HPP
#define ROUTES_STATS_ROUTE_HPP
#include <router.hpp>
#include <statman>

class Stats_route {

public:
  static void on_get(server::Request_ptr, server::Response_ptr res) {

    INFO("Stats_route","Getting stats \n");
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    Statman& statman = Statman::get();

    writer.StartArray();

    for(auto it = statman.begin(); it != statman.last_used(); ++it) {
      auto& stat = *it;
      writer.StartObject();

      writer.Key("name");
      writer.String(stat.name());

      writer.Key("value");
      std::string type = "";

      switch(stat.type()) {
        case Stat::UINT64:  writer.Uint(stat.get_uint64());
                            type = "UINT64";
                            break;
        case Stat::UINT32:  writer.Uint(stat.get_uint32());
                            type = "UINT32";
                            break;
        case Stat::FLOAT:   writer.Double(stat.get_float());
                            type = "FLOAT";
                            break;
      }

      writer.Key("type");
      writer.String(type);

      writer.Key("index");
      writer.Int(stat.index());

      writer.EndObject();
    }

    writer.EndArray();
    res->send_json(sb.GetString());
  }

};

#endif  // < ROUTES_STATS_ROUTE_HPP
