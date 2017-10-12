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

#ifndef MENDER_KEYSTORE_HPP
#define MENDER_KEYSTORE_HPP

#include "common.hpp"
#include <botan/rsa.h> // RSA_PublicKey, RSA_PrivateKey

namespace mender {

  using Public_key  = Botan::RSA_PublicKey; // RSA_PublicKey
  using Private_key = Botan::RSA_PrivateKey; // RSA_PrivateKey
  using Public_PEM  = std::string;
  using Auth_token  = byte_seq;

  class Keystore {
  public:
    static constexpr size_t KEY_LENGTH = 2048;

  public:
    Keystore(Storage store = nullptr);
    Keystore(Storage store, std::string name);
    Keystore(std::string key);

    byte_seq sign(const byte_seq& data);

    Public_PEM public_PEM();

    const Public_key& public_key()
    { return *private_key_; }

    void load();

    void save();

    void generate();

  private:
    Storage store_;
    std::unique_ptr<Private_key> private_key_;
    std::string key_name_;

    Public_key* load_from_PEM(fs::Buffer&) const;

  };



} // < namespace mender

#endif
