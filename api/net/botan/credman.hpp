// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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
#ifndef NET_TLS_CREDMAN_HPP
#define NET_TLS_CREDMAN_HPP

#include <botan/credentials_manager.h>
#include <botan/pk_algs.h>
#include <botan/rng.h>
#include <botan/x509cert.h>
#include <botan/x509_ca.h>
#include <botan/x509self.h>
#include <memory>

namespace net
{
typedef std::chrono::duration<int, std::ratio<31556926>> years;

class Credman : public Botan::Credentials_Manager
{
public:
  Credman(const Botan::X509_Certificate server_cert,
          const Botan::X509_Certificate ca_cert,
          std::unique_ptr<Botan::Private_Key> server_key) :
          m_server_cert(std::move(server_cert)),
          m_ca_cert(std::move(ca_cert)),
          m_server_key(std::move(server_key))
  {
    std::unique_ptr<Botan::Certificate_Store> store(new Botan::Certificate_Store_In_Memory(m_ca_cert));
    m_stores.push_back(std::move(store));
    m_provides_client_certs = false;
  }

  std::vector<Botan::Certificate_Store*>
  trusted_certificate_authorities(const std::string&,
          const std::string&) override
  {
    std::vector<Botan::Certificate_Store*> v;
    for (auto&& store : m_stores)
        v.push_back(store.get());
    return v;
  }

  std::vector<Botan::X509_Certificate> cert_chain(
              const std::vector<std::string>& cert_key_types,
              const std::string&,
              const std::string&) override
  {
    std::vector<Botan::X509_Certificate> chain;

    bool have_match = false;
    for (size_t i = 0; i != cert_key_types.size(); ++i)
        if(cert_key_types[i] == m_server_key->algo_name())
            have_match = true;

    if(have_match)
    {
      chain.push_back(m_server_cert);
      chain.push_back(m_ca_cert);
    }

    return chain;
  }

  Botan::Private_Key* private_key_for(const Botan::X509_Certificate&,
              const std::string&,
              const std::string&) override
  {
    return m_server_key.get();
  }

  static Credman* create(
        const std::string& name,
        Botan::RandomNumberGenerator&  rng,
        std::unique_ptr<Botan::Private_Key> ca_key,
        Botan::X509_Certificate ca_cert,
        std::unique_ptr<Botan::Private_Key> server_key);

public:
  Botan::X509_Certificate             m_server_cert, m_ca_cert;
  std::unique_ptr<Botan::Private_Key> m_server_key;
  std::vector<std::unique_ptr<Botan::Certificate_Store>> m_stores;
  bool m_provides_client_certs;
};

/**
 * 3. create private key 2 <server>
 * 4. create certificate request <req> with private key 2
 * 5. create CA with <CA> key and <CA> cert
 * 6, create certificate <server> by signing <req>
 *
**/
inline Credman* Credman::create(
        const std::string& server_name,
        Botan::RandomNumberGenerator&  rng,
        std::unique_ptr<Botan::Private_Key> ca_key,
        Botan::X509_Certificate ca_cert,
        std::unique_ptr<Botan::Private_Key> server_key)
{
  Botan::X509_CA ca(ca_cert, *ca_key, "SHA-512", rng);

  // create server certificate from CA
  auto now = std::chrono::system_clock::now();
  Botan::X509_Time start_time(now);
  Botan::X509_Time end_time(now + years(1));

  // create certificate request
  Botan::X509_Cert_Options server_opts;
  server_opts.common_name = server_name;
  server_opts.country = "VT";

  auto req = Botan::X509::create_cert_req(server_opts, *server_key, "SHA-512", rng);

  auto server_cert = ca.sign_request(req, rng, start_time, end_time);

  // create credentials manager
  return new Credman(server_cert, ca_cert, std::move(server_key));
}

} // net

#endif
