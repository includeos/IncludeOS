#include "balancer.hpp"
#include <memdisk>
#include <net/openssl/init.hpp>
#include <net/openssl/tls_stream.hpp>
#include <net/tcp/stream.hpp>

namespace microLB
{
  Balancer::Balancer(
        netstack_t& in,
        uint16_t    port,
        netstack_t& out,
        const std::string& tls_cert,
        const std::string& tls_key)
    : nodes(), netin(in), netout(out), signal({this, &Balancer::handle_queue})
  {
    fs::memdisk().init_fs(
    [] (fs::error_t err, fs::File_system&) {
      assert(!err);
    });

    openssl::init();
    openssl::verify_rng();

    this->openssl_data = openssl::create_server(tls_cert, tls_key);

    netin.tcp().listen(port,
      [this] (net::tcp::Connection_ptr conn) {
        if (conn != nullptr)
        {
          auto* stream = new openssl::TLS_stream(
              (SSL_CTX*) this->openssl_data,
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

    this->init_liveupdate();
  }
}
