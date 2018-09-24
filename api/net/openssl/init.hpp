#pragma once
#include <openssl/ossl_typ.h>
#include <fs/common.hpp>

namespace openssl
{
  extern void setup_rng();
  extern void verify_rng();
  extern void init();

  extern SSL_CTX* create_server(const std::string& cert, const std::string& key);

  extern SSL_CTX* create_client(fs::List, bool verify_peer = false);
  // enable peer certificate verification
  extern void client_verify_peer(SSL_CTX*);
}
