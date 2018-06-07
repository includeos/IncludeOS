#include <net/https/openssl_server.hpp>
#include <net/openssl/init.hpp>
#include <net/openssl/tls_stream.hpp>
#include <memdisk>

namespace http
{
  void OpenSSL_server::openssl_initialize(const std::string& certif,
                                          const std::string& key)
  {
    fs::memdisk().init_fs(
    [] (fs::error_t err, fs::File_system&) {
      assert(!err);
    });
    /** INIT OPENSSL **/
    openssl::init();
    /** SETUP CUSTOM RNG **/
    //openssl::setup_rng();
    /** VERIFY RNG **/
    openssl::verify_rng();

    this->m_ctx = openssl::create_server(certif.c_str(), key.c_str());
    assert(ERR_get_error() == 0);
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
    connect(
      std::make_unique<openssl::TLS_stream> ((SSL_CTX*) m_ctx, std::make_unique<net::tcp::Stream>(std::move(conn)))
    );
  }
} // http
