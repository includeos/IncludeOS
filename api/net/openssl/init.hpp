#pragma once
#include <openssl/ossl_typ.h>

namespace openssl
{
  extern void setup_rng();
  extern void verify_rng();
  extern void init();

  extern SSL_CTX* create_client(const char* path, bool verify_peer = false);
  // enable peer certificate verification
  extern void client_verify_peer(SSL_CTX*);
}
