#include "openssl_init.hpp"

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <kernel/rng.hpp>
#include <cassert>

extern "C" void ios_rand_seed(const void* buf, int num)
{
  rng_absorb(buf, num);
}
extern "C" int ios_rand_bytes(unsigned char* buf, int num)
{
  rng_extract(buf, num);
  return 1;
}
extern "C" void ios_rand_cleanup()
{
  /** do nothing **/
}
extern "C" void ios_rand_add(const void* buf, int num, double)
{
  rng_absorb(buf, num);
}
extern "C" int ios_rand_pseudorand(unsigned char* buf, int num)
{
  rng_absorb(buf, num);
  return 1;
}
extern "C" int ios_rand_status()
{
  return 1;
}

namespace openssl
{
  void setup_rng()
  {
    RAND_METHOD ios_rand {
      ios_rand_seed,
      ios_rand_bytes,
      ios_rand_cleanup,
      ios_rand_add,
      ios_rand_pseudorand,
      ios_rand_status
    };
    RAND_set_rand_method(&ios_rand);
  }
  void verify_rng()
  {
    auto* rm = RAND_get_rand_method();
    int random_value = 0;
    int rc = RAND_bytes((uint8_t*) &random_value, sizeof(random_value));
    assert(rc == 0 || rc == 1);
  }

  /* SSL library initialisation */
  void init()
  {
    static bool init_once = false;
    if (init_once == false)
    {
      init_once = true;
      SSL_library_init();
      OpenSSL_add_all_algorithms();
      SSL_load_error_strings();
      ERR_load_BIO_strings();
      ERR_load_crypto_strings();
    }
  }
}
