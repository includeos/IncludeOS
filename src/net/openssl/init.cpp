#include <net/openssl/init.hpp>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/engine.h>
#include <kernel/rng.hpp>
#include <cassert>
#include <info>

extern "C" int ios_rand_seed(const void* buf, int num)
{
  rng_absorb(buf, num);
  return 1;
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
extern "C" int ios_rand_add(const void* buf, int num, double)
{
  rng_absorb(buf, num);
  return 1;
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
    static RAND_METHOD ios_rand {
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
      INFO("OpenSSL", "Initializing (%s)", OPENSSL_VERSION_TEXT);
      init_once = true;
      printf("setup_rng\n");
      setup_rng();
      printf("SSL_library_init\n");
      SSL_library_init(); // OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS
      printf("SSL_load_error_strings\n");
      SSL_load_error_strings();
      printf("ERR_load_crypto_strings\n");
      ERR_load_crypto_strings();
      printf("ERR_load_BIO_strings\n");
      ERR_load_BIO_strings();
    }
  }
}
