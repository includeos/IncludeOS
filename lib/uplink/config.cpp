// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 Oslo and Akershus University College of Applied Sciences
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

#include <config>
#include "config.hpp"
#include "common.hpp"
#include <net/super_stack.hpp>

#ifndef RAPIDJSON_HAS_STDSTRING
  #define RAPIDJSON_HAS_STDSTRING 1
#endif

#ifndef RAPIDJSON_THROWPARSEEXCEPTION
  #define RAPIDJSON_THROWPARSEEXCEPTION 1
#endif

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

namespace uplink {

  Config Config::read()
  {
    MYINFO("Reading uplink config");

    const auto& json = ::Config::get();

    Expects(not json.empty() && "Config is empty");

    using namespace rapidjson;
    Document doc;
    doc.Parse(json.data());

    Expects(doc.IsObject() && "Malformed config");

    Expects(doc.HasMember("uplink") && "Missing member \"uplink\"");

    auto& cfg = doc["uplink"];

    Expects(cfg.HasMember("url") && cfg.HasMember("token") && "Missing url or/and token");

    Config config_;

    config_.url   = cfg["url"].GetString();
    config_.token = cfg["token"].GetString();

    // Decide stack/interface
    if(cfg.HasMember("index"))
    {
      auto& index = cfg["index"];

      if(index.IsNumber())
      {
        config_.inet = &net::Super_stack::get<net::IP4>(index.GetInt());
      }
      else
      {
        config_.inet = &net::Super_stack::get<net::IP4>(index.GetString());
      }
    }
    // If not given, pick the first stack
    else
    {
      config_.inet = &net::Super_stack::get<net::IP4>(0);
    }

    // Reboot on panic (optional)
    if(cfg.HasMember("reboot"))
    {
      config_.reboot = cfg["reboot"].GetBool();
    }

    // Log over websocket (optional)
    if(cfg.HasMember("ws_logging"))
    {
      config_.ws_logging = cfg["ws_logging"].GetBool();
    }

    // Serialize conntrack
    if(cfg.HasMember("serialize_ct"))
    {
      config_.serialize_ct = cfg["serialize_ct"].GetBool();
    }

    if(cfg.HasMember("tag"))
    {
      config_.tag = cfg["tag"].GetString();
    }

    return config_;
  }

}
