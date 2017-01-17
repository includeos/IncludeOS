// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
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

#ifndef MENDER_AUTH_HPP
#define MENDER_AUTH_HPP

#include "common.hpp"
#include "json.hpp"

namespace mender {

  /** Device identifier. e.g. JSON(MAC_addr) */
  using Dev_id      = std::string;
  /** Public key */
  using Public_PEM  = std::string;
  /** Token */
  using Auth_token  = byte_seq;

  using Writer      = std::string; // rapidjson::Writer


  /** Authorization request data, built by the caller (Auth_manager) */
  struct Auth_request_data {
    Dev_id      id_data;
    Auth_token  tenant_token;
    Public_PEM  pubkey;
    int64_t     seq_no;

    inline byte_seq serialized_bytes() const;

  private:
    template <typename Writer>
    void serialize(Writer& wr) const;
  };

  inline byte_seq Auth_request_data::serialized_bytes() const
  {
    nlohmann::json j;
    j["tenant_token"] = std::string{tenant_token.begin(), tenant_token.end()};
    j["seq_no"]       = seq_no;
    j["id_data"]      = id_data;
    j["pubkey"]       = pubkey;

    auto str = j.dump();
    //printf("%s\n", str.c_str());
    return {str.begin(), str.end()};
  }

  template <typename Writer>
  void Auth_request_data::serialize(Writer& wr) const
  {
    (void)wr;
    // this one is when using rapidjson
  }

  /** An authorization request */
  struct Auth_request {
    byte_seq   data;
    Auth_token token;
    byte_seq   signature;
  };

} // < namespace mender

#endif
