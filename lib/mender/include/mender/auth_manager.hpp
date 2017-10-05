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

#ifndef MENDER_AUTH_MANAGER_HPP
#define MENDER_AUTH_MANAGER_HPP

#include "auth.hpp"
#include "keystore.hpp"
#include <delegate>

namespace mender {

  class Auth_manager {
  private:
    using Request         = Auth_request;
    using Request_data    = Auth_request_data;

  public:

    Auth_manager(std::unique_ptr<Keystore> ks, Dev_id id);

    Auth_request make_auth_request();

    Auth_token auth_token() const
    { return tenant_token_; }

    void set_auth_token(byte_seq token)
    { tenant_token_ = std::move(token); }

    Keystore& keystore()
    { return *keystore_; }

  private:
    std::unique_ptr<Keystore>   keystore_;
    const Dev_id                id_data_;
    Auth_token                  tenant_token_;

  };

} // < namespace mender

#endif

