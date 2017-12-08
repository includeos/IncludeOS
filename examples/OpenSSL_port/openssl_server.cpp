#include "openssl_server.hpp"
#include "tls_stream.hpp"
#include "openssl_init.hpp"
#include <memdisk>
#define LOAD_FROM_MEMDISK

namespace http
{
  // https://gist.github.com/darrenjs/4645f115d10aa4b5cebf57483ec82eca
  inline void handle_error(const char* file, int lineno, const char* msg) {
    fprintf(stderr, "** %s:%i %s\n", file, lineno, msg);
    ERR_print_errors_fp(stderr);
    exit(1);
  }
  #define int_error(msg) handle_error(__FILE__, __LINE__, msg)

  static void
  tls_load_from_memory(SSL_CTX* ctx,
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

    int error = ERR_get_error();
    if (error) {
      printf("Status: %s\n", ERR_error_string(error, nullptr));
    }
    assert(error == SSL_ERROR_NONE);
    return ctx;
  }

  void OpenSSL_server::openssl_initialize(const std::string& certif,
                                          const std::string& key)
  {
    fs::memdisk().init_fs(
    [] (auto err, auto&) {
      assert(!err);
    });
    /** INIT OPENSSL **/
    openssl::init();
    /** SETUP CUSTOM RNG **/
    //openssl::setup_rng();
    /** VERIFY RNG **/
    openssl::verify_rng();

    this->m_ctx = tls_init_server(certif.c_str(), key.c_str());
  }
  OpenSSL_server::~OpenSSL_server()
  {
    SSL_CTX_free((SSL_CTX*) this->m_ctx);
  }

  void OpenSSL_server::bind(const uint16_t port)
  {
    tcp_.listen(port, {this, &OpenSSL_server::on_connect});
    INFO("HTTPS Server", "Listening on port %u", port);
  }

  void OpenSSL_server::on_connect(TCP_conn conn)
  {
    auto* ptr = new openssl::TLS_stream((SSL_CTX*) m_ctx, std::move(conn));

    ptr->on_connect(
    [this, ptr] (net::Stream&)
    {
      // create and pass TLS socket
      connect(std::unique_ptr<openssl::TLS_stream>(ptr));
    });

    // this is ok due to the created Server_connection inside
    // connect assigns a new on_close
    ptr->on_close([ptr] {
      delete ptr;
    });
  }

} // http
