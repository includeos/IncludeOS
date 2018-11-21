#include "balancer.hpp"
#include <memdisk>
#include <net/openssl/init.hpp>
#include <net/openssl/tls_stream.hpp>
#include <net/tcp/stream.hpp>

namespace microLB
{
  void Balancer::open_for_ossl(
        netstack_t&        interface,
        const uint16_t     client_port,
        const std::string& tls_cert,
        const std::string& tls_key)
  {
    fs::memdisk().init_fs(
    [] (fs::error_t err, fs::File_system&) {
      assert(!err);
    });

    openssl::init();
    openssl::verify_rng();

    this->tls_context = openssl::create_server(tls_cert, tls_key);

    interface.tcp().listen(client_port,
      [this] (net::tcp::Connection_ptr conn) {
        if (conn != nullptr)
        {
          auto* stream = new openssl::TLS_stream(
              (SSL_CTX*) this->tls_context,
              std::make_unique<net::tcp::Stream>(conn)
          );
          stream->on_connect(
            [this, stream] (auto&) {
              this->incoming(std::unique_ptr<openssl::TLS_stream> (stream));
            });
          stream->on_close(
            [stream] () {
              delete stream;
            });
        }
      });
  } // open_ossl(...)
}
