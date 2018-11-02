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

#pragma once
#ifndef UPLINK_CONFIG_HPP
#define UPLINK_CONFIG_HPP

#include <string>
#include <net/inet>
#include <uri>

namespace uplink {

  const static std::string default_cert_path{"/certs"};

  struct Config
  {
    std::string index_str;
    int         index{-1};
    uri::URI    url;
    std::string token;
    std::string tag;
    std::string certs_path    = default_cert_path;
    bool        verify_certs  = true;
    bool        reboot        = true;
    bool        ws_logging    = true;
    bool        serialize_ct  = false;

    static Config read();

    std::string serialized_string() const;

    net::Inet& get_stack() const;
  };

}

#endif
