#include <net/openssl/init.hpp>
#include <net/openssl/tls_stream.hpp>
#include <openssl/rsa.h>
#include <memdisk>
#define LOAD_FROM_MEMDISK

// https://gist.github.com/darrenjs/4645f115d10aa4b5cebf57483ec82eca
inline void handle_error(const char* file, int lineno, const char* msg) {
  fprintf(stderr, "** %s:%i %s\n", file, lineno, msg);
  ERR_print_errors_fp(stderr);
  exit(1);
}
#define int_error(msg) handle_error(__FILE__, __LINE__, msg)

static void
tls_check_for_errors()
{
  int error = ERR_get_error();
  if (error) {
    printf("Status: %s\n", ERR_error_string(error, nullptr));
  }
  assert(error == SSL_ERROR_NONE);
}

static void
tls_load_from_memory(X509_STORE* store,
                     fs::Buffer cert_buffer)
{
  auto* cbio = BIO_new_mem_buf(cert_buffer.data(), cert_buffer.size());
  auto* cert = PEM_read_bio_X509(cbio, NULL, 0, NULL);
  assert(cert != NULL);
  int res = X509_STORE_add_cert(store, cert);
  assert(res == 1);
  BIO_free(cbio);
}

static void
tls_private_key_for_ctx(SSL_CTX* ctx, int bits = 2048)
{
  BIGNUM* bne = BN_new();
  assert(BN_set_word(bne, RSA_F4) == 1);

  RSA* rsa = RSA_new();
  int ret = RSA_generate_key_ex(rsa, bits, bne, NULL);
  assert(ret == 1);

  SSL_CTX_use_RSAPrivateKey(ctx, rsa);
}

static SSL_CTX*
tls_init_client(const char* path)
{
  /* create the SSL server context */
  auto meth = TLSv1_2_method();
  auto* ctx = SSL_CTX_new(meth);
  if (!ctx) throw std::runtime_error("SSL_CTX_new()");

  int res = SSL_CTX_set_cipher_list(ctx, "AES256-SHA");
  assert(res == 1);

  X509_STORE* store = X509_STORE_new();
  assert(store != nullptr);

#ifdef LOAD_FROM_MEMDISK
  auto& filesys = fs::memdisk().fs();
  auto ents = filesys.ls(path);
  for (auto& ent : ents)
  {
    if (ent.is_file())
    {
      printf("Loading cert %s\n", ent.name().c_str());
      auto buffer = ent.read(0, ent.size());
      tls_load_from_memory(store, buffer);
    }
  }
#else
#   error "Implement me"
#endif

  // assign CA store to CTX
  SSL_CTX_set_cert_store(ctx, store);

  // create private key for client
  tls_private_key_for_ctx(ctx);

  /* Recommended to avoid SSLv2 & SSLv3 */
  SSL_CTX_set_options(ctx, SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3);

  tls_check_for_errors();
  return ctx;
}

namespace openssl
{
  SSL_CTX* create_client(const char* path, bool verify_peer)
  {
    auto* ctx = tls_init_client(path);
    if (verify_peer) client_verify_peer(ctx);
    return ctx;
  }

  void client_verify_peer(SSL_CTX* ctx)
  {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
    SSL_CTX_set_verify_depth(ctx, 1);
    tls_check_for_errors();
  }
}
