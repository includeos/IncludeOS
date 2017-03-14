// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

//#include <net/http/tls_server.hpp>
#include <net/http/secure_server.hpp>
#include <botan/system_rng.h>
#include <botan/pkcs8.h>

inline static auto& get_rng() { return Botan::system_rng(); }

inline std::unique_ptr<Botan::Private_Key> read_pkey(fs::Dirent& key_file)
{
  assert(key_file.is_file());
  Botan::DataSource_Memory data{key_file.read()};
  return std::unique_ptr<Botan::Private_Key>(Botan::PKCS8::load_key(data, get_rng()));
}

namespace http
{
  Secure_server::Secure_server(
      fs::Dirent& file_ca_key,
      fs::Dirent& file_ca_cert,
      fs::Dirent& file_server_key,
      TCP& tcp,
      Request_handler cb)
    : http::Server(tcp, cb), 
      rng(get_rng())
  {
    on_connect = {this, &Secure_server::secure_connect};

    // load CA certificate
    assert(file_ca_cert.is_valid());
    auto ca_cert = file_ca_cert.read();
    std::vector<uint8_t> vca_cert(ca_cert.begin(), ca_cert.end());
    // load CA private key
    auto ca_key = read_pkey(file_ca_key);
    // load server private key
    auto srv_key = read_pkey(file_server_key);

    auto* credman = net::Credman::create(
            get_rng(),
            std::move(ca_key),
            Botan::X509_Certificate(vca_cert),
            std::move(srv_key));

    this->credman.reset(credman);
  }
}
