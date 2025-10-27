#include <net/openssl/init.hpp>
#include <net/openssl/tls_stream.hpp>
#include <memdisk>
#include <cassert>
#define LOAD_FROM_MEMDISK

namespace openssl
{
  // https://gist.github.com/darrenjs/4645f115d10aa4b5cebf57483ec82eca
  inline static void
  handle_error(const char* file, int lineno, const char* msg) {
    fprintf(stderr, "** %s:%i %s\n", file, lineno, msg);
    ERR_print_errors_fp(stderr);
    exit(1);
  }
  #define int_error(msg) handle_error(__FILE__, __LINE__, msg)

  static void
  load_from_memory(SSL_CTX* ctx,
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

    // TODO: -Wdeprecated-declarations
    SSL_CTX_use_RSAPrivateKey(ctx, key);
    BIO_free(kbio);
  }

  SSL_CTX* create_server(const std::string& cert_file,
                         const std::string& key_file)
  {
    /* create the SSL server context */
    // TODO: -Wdeprecated-declarations
    auto* ctx = SSL_CTX_new(TLSv1_2_method());
    if (!ctx) throw std::runtime_error("SSL_CTX_new()");

    [[maybe_unused]] int res = SSL_CTX_set_cipher_list(ctx, "AES256-SHA");
    assert(res == 1);

  #ifdef LOAD_FROM_MEMDISK
    auto& filesys = fs::memdisk().fs();
    // load CA certificate
    auto ca_cert_buffer = filesys.read_file(cert_file);
    // load CA private key
    auto ca_key_buffer  = filesys.read_file(key_file);
    // use in SSL CTX
    load_from_memory(ctx, ca_cert_buffer, ca_key_buffer);
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

    int error = ERR_get_error();
    if (error) {
      printf("Status: %s\n", ERR_error_string(error, nullptr));
    }
    assert(error == SSL_ERROR_NONE);
    return ctx;
  }
}
