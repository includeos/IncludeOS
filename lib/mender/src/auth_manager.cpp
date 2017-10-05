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

#include <mender/auth_manager.hpp>

namespace mender {

  Auth_manager::Auth_manager(std::unique_ptr<Keystore> ks, Dev_id id)
    : keystore_(std::move(ks)),
      id_data_(std::move(id))
  {
    MENDER_INFO("Auth_manager", "Created with identity: %s", id_data_.c_str());
    std::string tkn{""};
    tenant_token_ = Auth_token{tkn.begin(), tkn.end()};
  }

  Auth_request Auth_manager::make_auth_request()
  {
    Request_data authd;

    // Populate request data
    authd.id_data       = this->id_data_;
    authd.pubkey        = this->keystore_->public_PEM();
    authd.tenant_token  = this->tenant_token_;

    // Get serialized bytes
    const auto reqdata = authd.serialized_bytes();

    // Generate signature from payload
    const auto signature = keystore_->sign(reqdata);

    return Request {
      .data       = reqdata,
      .token      = this->tenant_token_,
      .signature  = signature
    };
  }

}

