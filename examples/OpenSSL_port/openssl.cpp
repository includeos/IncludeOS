#include "tls_stream.hpp"
#include <openssl/rand.h>
#include <memdisk>
#define LOAD_FROM_MEMDISK

// https://gist.github.com/darrenjs/4645f115d10aa4b5cebf57483ec82eca
inline void handle_error(const char* file, int lineno, const char* msg) {
  fprintf(stderr, "** %s:%i %s\n", file, lineno, msg);
  ERR_print_errors_fp(stderr);
  exit(1);
}
#define int_error(msg) handle_error(__FILE__, __LINE__, msg)

void tls_load_from_memory(SSL_CTX* ctx,
                          fs::Buffer cert_buffer,
                          fs::Buffer key_buffer)
{
  auto* cbio = BIO_new_mem_buf(cert_buffer.data(), cert_buffer.size());
  auto* cert = PEM_read_bio_X509(cbio, NULL, 0, NULL);
  assert(cert != NULL);
  SSL_CTX_use_certificate(ctx, cert);
  BIO_free(cbio);

  auto* kbio = BIO_new_mem_buf(key_buffer.data(), key_buffer.size());
  auto* key = PEM_read_bio_RSAPrivateKey(kbio, NULL, 0, NULL);
  assert(key != NULL);
  SSL_CTX_use_RSAPrivateKey(ctx, key);
  BIO_free(kbio);
}

void openssl_init()
{
  printf("Initialising OpenSSL\n");
  /* SSL library initialisation */
  SSL_library_init();
  OpenSSL_add_all_algorithms();
  SSL_load_error_strings();
  ERR_load_BIO_strings();
  ERR_load_crypto_strings();
}

SSL_CTX* tls_init_server(const char* cert_file, const char* key_file)
{
  /* create the SSL server context */
  auto meth = TLSv1_1_method();
  auto* ctx = SSL_CTX_new(meth);
  if (!ctx) throw std::runtime_error("SSL_CTX_new()");

  int res = SSL_CTX_set_cipher_list(ctx, "AES256-SHA");
  assert(res == 1);

#ifdef LOAD_FROM_MEMDISK
  auto& filesys = fs::memdisk().fs();
  // load CA certificate
  auto ca_cert_buffer = filesys.read_file(cert_file);
  // load CA private key
  auto ca_key_buffer  = filesys.read_file(key_file);
  // use in SSL CTX
  tls_load_from_memory(ctx, ca_cert_buffer, ca_key_buffer);
#else
  /* Load certificate and private key files, and check consistency  */
  int err;
  err = SSL_CTX_use_certificate_file(ctx, cert_file,  SSL_FILETYPE_PEM);
  if (err != 1)
    int_error("SSL_CTX_use_certificate_file failed");

  /* Indicate the key file to be used */
  err = SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM);
  if (err != 1)
    int_error("SSL_CTX_use_PrivateKey_file failed");
#endif

  /* Make sure the key and certificate file match. */
  if (SSL_CTX_check_private_key(ctx) != 1)
    int_error("SSL_CTX_check_private_key failed");

  /* Recommended to avoid SSLv2 & SSLv3 */
  SSL_CTX_set_options(ctx, SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3);
  return ctx;
}

#include <kernel/rng.hpp>
extern "C" void ios_rand_seed(const void* buf, int num)
{
  printf("RAND_seed called\n");
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
  printf("ios_rand_add %p:%d\n", buf, num);
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

void openssl_setup_rng()
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
void openssl_verify_rng()
{
  auto* rm = RAND_get_rand_method();
  int random_value = 0;
  int rc = RAND_bytes((uint8_t*) &random_value, sizeof(random_value));
  assert(rc == 0 || rc == 1);
  printf("Random returned %d\n", random_value);
}

void openssl_server_test()
{
  fs::memdisk().init_fs(
  [] (auto err, auto&) {
    assert(!err);

    /** INIT OPENSSL **/
    openssl_init();
    /** SETUP CUSTOM RNG **/
    //openssl_setup_rng();
    /** VERIFY RNG **/
    openssl_verify_rng();

    auto* ctx = tls_init_server("/test.pem", "/test.key");
    assert(ctx);
    printf("Done initializing OpenSSL context\n");
    int error = ERR_get_error();
    printf("Status: %d\n", error);
    assert(error == SSL_ERROR_NONE);

    error = ERR_get_error();
    if (error) {
      printf("Status: %s\n", ERR_error_string(error, nullptr));
      assert(error == 0);
    }

    auto& inet = net::Super_stack::get<net::IP4>(0);
    auto& server = inet.tcp().listen(443);
    server.on_connect(
      [ctx] (auto conn) {
        printf("Connected to %s\n", conn->to_string().c_str());
        auto tcp_stream = std::make_unique<net::tcp::Connection::Stream> (conn);
        auto* tls = new TLS_stream(ctx, std::move(tcp_stream));
        tls->on_read =
        [tls] (auto buffer) {
          printf("On_read: %.*s\n", (int) buffer->size(), buffer->data());
          tls->write("Hello world!\n\n");
        };
      });
    printf("Listening on 443\n");
  });

}
