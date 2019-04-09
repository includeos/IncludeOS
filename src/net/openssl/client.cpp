#include <net/openssl/init.hpp>
#include <net/openssl/tls_stream.hpp>
#include <openssl/rsa.h>
#include <fs/dirent.hpp>
#include <info>

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
  assert(cert != NULL && "Invalid certificate");
  int res = X509_STORE_add_cert(store, cert);
  assert(res == 1 && "The X509 store did not accept the certificate");
  BIO_free(cbio);
}

static void
tls_private_key_for_ctx(SSL_CTX* ctx, int bits = 2048)
{
  BIGNUM* bne = BN_new();
  auto res = BN_set_word(bne, RSA_F4);
  assert(res == 1);
  (void)res;

  RSA* rsa = RSA_new();
  int ret = RSA_generate_key_ex(rsa, bits, bne, NULL);
  assert(ret == 1);

  ret = SSL_CTX_use_RSAPrivateKey(ctx, rsa);
  assert(ret == 1 && "OpenSSL context did not accept the private key");
}

static SSL_CTX*
tls_init_client(fs::List ents)
{
  /* create the SSL server context */
  auto meth = TLSv1_2_method();
  auto* ctx = SSL_CTX_new(meth);
  if (!ctx) throw std::runtime_error("SSL_CTX_new()");

  int res = SSL_CTX_set_cipher_list(ctx, "AES256-SHA");
  assert(res == 1);

  X509_STORE* store = X509_STORE_new();
  assert(store != nullptr);

  int certs = 0;
  for (auto& ent : ents)
  {
    if (ent.is_file())
    {
      TLS_PRINT("\t\t* %s\n", ent.name().c_str());
      auto buffer = ent.read(0, ent.size());
      tls_load_from_memory(store, buffer);
      certs++;
    }
  }
  INFO2("Loaded %d certificates", certs);

  // assign CA store to CTX
  SSL_CTX_set_cert_store(ctx, store);

  //SSL_CTX_load_verify_locations(ctx, nullptr, "/mozilla");

  // create private key for client
  tls_private_key_for_ctx(ctx);

  /* Recommended to avoid SSLv2 & SSLv3 */
  SSL_CTX_set_options(ctx, SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3);

  tls_check_for_errors();
  return ctx;
}

extern "C" typedef int verify_cb_t(int, X509_STORE_CTX*);
static verify_cb_t verify_cb;
int verify_cb(int preverify, X509_STORE_CTX* ctx)
{
  X509* current_cert = X509_STORE_CTX_get_current_cert(ctx);

  if (preverify == 0)
  {
    if (current_cert) {
      X509_NAME_print_ex_fp(stdout,
                            X509_get_subject_name(current_cert),
                            0, XN_FLAG_ONELINE);
      printf("\n");
    }
    TLS_PRINT("verify_cb error: %d\n", X509_STORE_CTX_get_error(ctx));
    ERR_print_errors_fp(stdout);
  }
  return preverify;
}

namespace openssl
{
  SSL_CTX* create_client(fs::List ents, bool verify_peer)
  {
    INFO("OpenSSL", "Initializing client context");
    auto* ctx = tls_init_client(ents);
    CHECK(verify_peer, "Verify peer");
    if (verify_peer) client_verify_peer(ctx);
    return ctx;
  }

  void client_verify_peer(SSL_CTX* ctx)
  {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER|SSL_VERIFY_CLIENT_ONCE, &verify_cb);
    SSL_CTX_set_verify_depth(ctx, 20);
    tls_check_for_errors();
  }
}
