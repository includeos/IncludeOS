#pragma once
#include <openssl/ossl_typ.h>

namespace openssl
{
  extern void setup_rng();
  extern void verify_rng();
  extern void init();

  extern SSL_CTX* create_client(const char* path);
}
