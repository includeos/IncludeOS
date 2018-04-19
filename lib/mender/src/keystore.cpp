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

#include <mender/keystore.hpp>

#include <botan/x509cert.h>
#include <botan/data_src.h>
#include <botan/pkcs8.h>
#include <botan/pubkey.h>
#include <botan/hex.h>
#include <kernel/botan_rng.hpp>

namespace mender {

  Keystore::Keystore(Storage store)
    : store_{std::move(store)}
  {
    MENDER_INFO("Keystore", "Constructing keystore with a shared disk");
    generate();
  }

  Keystore::Keystore(Storage store, std::string kname)
    : store_{std::move(store)},
      key_name_{std::move(kname)}
  {
    MENDER_INFO("Keystore", "Constructing keystore with a shared disk and keyname");
    load();
  }

  Keystore::Keystore(std::string key)
    : store_(nullptr),
    key_name_{""}
  {
    MENDER_INFO("Keystore", "Constructing keystore with the private key itself");
    Botan::DataSource_Memory data{key};
    private_key_.reset(dynamic_cast<Private_key*>(Botan::PKCS8::load_key(data, IncludeOS_RNG::get())));
    //assert(private_key_->check_key(IncludeOS_RNG::get(), true));
  }

  void Keystore::load()
  {
    MENDER_INFO("Keystore", "Loading private key from \"%s\"...", key_name_.c_str());
    auto buffer = store_->fs().read_file(key_name_);
    if(buffer)
    {
      Botan::DataSource_Memory data{buffer.data(), buffer.size()};
      private_key_.reset(dynamic_cast<Private_key*>(Botan::PKCS8::load_key(data, IncludeOS_RNG::get())));
      MENDER_INFO2("%s\n ... Done.", Botan::PKCS8::PEM_encode(*private_key_).c_str());
    }
    else
    {
      generate();
    }
  }

  void Keystore::save()
  {
    MENDER_INFO("Keystore", "Storing key to \"%s\"", key_name_.c_str());
    MENDER_INFO2("Writing is not supported, skipping.");
  }

  void Keystore::generate()
  {
    MENDER_INFO("Keystore", "Generating private key ...");
    private_key_ = std::make_unique<Private_key>(IncludeOS_RNG::get(), 1024);
    MENDER_INFO2("%s\n ... Done.", Botan::PKCS8::PEM_encode(*private_key_).c_str());
    save();
  }

  Public_PEM Keystore::public_PEM()
  {
    auto pem = Botan::X509::PEM_encode(public_key());
    //printf("PEM:\n%s\n", pem.c_str());
    //auto pem = Botan::PEM_Code::encode(data, "PUBLIC KEY");
    return pem;
  }

  byte_seq Keystore::sign(const byte_seq& data)
  {
    // https://botan.randombit.net/manual/pubkey.html#signatures
    // EMSA3 for backward compability with PKCS1_15
    Botan::PK_Signer signer(*private_key_, IncludeOS_RNG::get(), "EMSA3(SHA-256)");
    auto signature = signer.sign_message((uint8_t*)data.data(), data.size(), IncludeOS_RNG::get());
    //printf("Sign:\n%s\n", Botan::hex_encode(signature).c_str());
    return signature;
  }

}
