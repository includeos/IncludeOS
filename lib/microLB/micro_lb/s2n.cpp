#include "balancer.hpp"
#include <memdisk>
#include <net/s2n/stream.hpp>
#include <net/tcp/stream.hpp>

namespace microLB
{
  // allow all clients
  static uint8_t verify_host_passthrough(const char*, size_t, void* /*data*/) {
      return 1;
  }

  static s2n_config* s2n_create_config(
      const std::string& ca_cert,
      const std::string& ca_key)
  {
#ifdef __includeos__
    setenv("S2N_DONT_MLOCK", "0", 1);
#endif
    if (s2n_init() < 0) {
      s2n::print_s2n_error("Error running s2n_init()");
      exit(1);
    }
    
    s2n_config* config = s2n_config_new();
    assert(config != nullptr);
    
    int res =
    s2n_config_add_cert_chain_and_key(config, ca_cert.c_str(), ca_key.c_str());
    if (res < 0) {
      s2n::print_s2n_error("Error getting certificate/key");
      exit(1);
    }
    
    res =
    s2n_config_set_verify_host_callback(config, verify_host_passthrough, nullptr);
    if (res < 0) {
      s2n::print_s2n_error("Error setting verify-host callback");
      exit(1);
    }
    
    return config;
  }
  
  void Balancer::open_s2n(
        const uint16_t     client_port,
        const std::string& cert_path,
        const std::string& key_path)
  {
    fs::memdisk().init_fs(
    [] (fs::error_t err, fs::File_system&) {
      assert(!err);
    });
    auto ca_cert = fs::memdisk().fs().read_file(cert_path).to_string();
    auto ca_key  = fs::memdisk().fs().read_file(key_path).to_string();

    this->tls_context = s2n_create_config(ca_cert, ca_key);
    assert(this->tls_context != nullptr);

    this->tls_free = [this] () {
        s2n_config_free((s2n_config*) this->tls_context);
      };

    netin.tcp().listen(client_port,
      [this] (net::tcp::Connection_ptr conn) {
        if (conn != nullptr)
        {
          auto* stream = new s2n::TLS_stream(
              (s2n_config*) this->tls_context,
              std::make_unique<net::tcp::Stream>(conn),
              false
            );
          stream->on_connect(
            [this, stream] (auto&) {
              this->incoming(std::unique_ptr<s2n::TLS_stream> (stream));
            });
          stream->on_close(
            [stream] () {
              delete stream;
            });
        }
      });
  } // open_s2n(...)
}
