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

#pragma once
#ifndef NET_AUTOCONF_HPP
#define NET_AUTOCONF_HPP

#ifndef RAPIDJSON_HAS_STDSTRING
  #define RAPIDJSON_HAS_STDSTRING 1
#endif

#ifndef RAPIDJSON_THROWPARSEEXCEPTION
  #define RAPIDJSON_THROWPARSEEXCEPTION 1
#endif

#include <string>
#include <rapidjson/document.h>

namespace net {

/**
 * @brief      Automatically configures network interfaces from a config.
 *
 * Example for configuring interface 0 and 1:
 * {
 *   "net": [
 *      ["10.0.0.42", "255.255.255.0", "10.0.0.1"],
 *      "dhcp"
 *   ]
 * }
 */
struct autoconf
{
  const static std::string DEFAULT_CFG; // config.json

  /**
   * @brief      Try to load the default config from memdisk.
   */
  static void load();

  /**
   * @brief      Try to load a config with a specific name/path from memdisk.
   *
   * @param[in]  file  The config file to load
   */
  static void load(const std::string& file);

  /**
   * @brief      Configure interfaces according to the JSON.
   *
   * @param[in]  json  JSON representing the config
   */
  static void configure(const std::string& json);

  /**
   * @brief      Configure interfaces according the json object "net".
   *
   * @param[in]  net   A rapidjson value of the member "net"
   */
  static void configure(const rapidjson::Value& net);
};

}

#endif
