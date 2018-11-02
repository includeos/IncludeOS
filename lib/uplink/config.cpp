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
#include <net/inet.hpp>
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

    Expects(config_.url.is_valid() && "Invalid URL given (may have missing scheme)");
    Expects(not config_.url.scheme().empty() && "Scheme missing in URL (http or https)");

    // Decide stack/interface
    if(cfg.HasMember("index"))
    {
      auto& index = cfg["index"];

      if(index.IsNumber())
      {
        config_.index = index.GetInt();
      }
      else
      {
        config_.index_str = index.GetString();
      }
    }
    // If not given, pick the first stack
    else
    {
      config_.index = 0;
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

    if(cfg.HasMember("certs"))
    {
      config_.certs_path = cfg["certs"].GetString();
    }

    if(cfg.HasMember("verify"))
    {
      config_.verify_certs = cfg["verify"].GetBool();
    }

    return config_;
  }

  std::string Config::serialized_string() const
  {
    using namespace rapidjson;
    StringBuffer buf;
    Writer<StringBuffer> writer{buf};

    writer.StartObject();

    writer.Key("index");
    (index >= 0) ? writer.Int(index) : writer.String(index_str);

    writer.Key("url");
    writer.String(url);

    writer.Key("token");
    writer.String(token);

    writer.Key("tag");
    writer.String(tag);

    writer.Key("certs");
    writer.String(certs_path);

    writer.Key("verify");
    writer.Bool(verify_certs);

    writer.Key("reboot");
    writer.Bool(reboot);

    writer.Key("ws_logging");
    writer.Bool(ws_logging);

    writer.Key("serialize_ct");
    writer.Bool(serialize_ct);

    writer.EndObject();

    return buf.GetString();
  }

  net::Inet& Config::get_stack() const
  {
    return (index >= 0) ? net::Super_stack::get(index)
      : net::Super_stack::get(index_str);
  }

}
