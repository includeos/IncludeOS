// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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

#include <util/autoconf.hpp>

#ifndef RAPIDJSON_HAS_STDSTRING
  #define RAPIDJSON_HAS_STDSTRING 1
#endif

#ifndef RAPIDJSON_THROWPARSEEXCEPTION
  #define RAPIDJSON_THROWPARSEEXCEPTION 1
#endif

#include <rapidjson/document.h>
#include <util/config.hpp>
#include <net/configure.hpp>
#include <common>

void autoconf::run()
{
  INFO("Autoconf", "Running auto configure");

  const auto& cfg = Config::get();

  if(cfg.empty())
  {
    INFO2("No config found");
    return;
  }

  rapidjson::Document doc;
  doc.Parse(cfg.data());

  Expects(doc.IsObject() && "Malformed config (not an object)");

  // Configure network
  if(doc.HasMember("net"))
  {
    net::configure(doc["net"]);
  }

  INFO("Autoconf", "Finished");
}
