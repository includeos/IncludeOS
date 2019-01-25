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

#include <net/https/botan_server.hpp>

#include <kernel/botan_rng.hpp>
#include <botan/data_src.h>
#include <botan/pkcs8.h>

inline static Botan::RandomNumberGenerator& get_rng() {
  return IncludeOS_RNG::get();
}

inline std::unique_ptr<Botan::Private_Key> read_pkey(fs::Dirent& key_file)
{
  assert(key_file.is_file());
  Botan::DataSource_Memory data{key_file.read()};
  return std::unique_ptr<Botan::Private_Key>(Botan::PKCS8::load_key(data, get_rng()));
}

namespace http
{
  Botan::RandomNumberGenerator& Botan_server::get_rng()
  {
    return ::get_rng();
  }

  void Botan_server::load_credentials(
    const std::string& server_name,
    fs::Dirent& file_ca_key,
    fs::Dirent& file_ca_cert,
    fs::Dirent& file_server_key)
  {
    // load CA certificate
    assert(file_ca_cert.is_valid());
    auto ca_cert = file_ca_cert.read(0, file_ca_cert.size());
    assert(ca_cert.is_valid());

    // load CA private key
    auto ca_key = read_pkey(file_ca_key);
    // load server private key
    auto srv_key = read_pkey(file_server_key);

    auto* credman = net::Credman::create(
            server_name,
            get_rng(),
            std::move(ca_key),
            Botan::X509_Certificate(ca_cert.data(), ca_cert.size()),
            std::move(srv_key));

    this->credman.reset(credman);
  }

  void Botan_server::bind(const uint16_t port)
  {
    tcp_.listen(port, {this, &Botan_server::on_connect});
    INFO("HTTPS Server", "Listening on port %u", port);
  }

  void Botan_server::on_connect(TCP_conn conn)
  {
    connect(
      std::make_unique<net::botan::Server> (
        std::make_unique<net::tcp::Stream>(
          std::move(conn)), rng, *credman)
    );
  }

}
