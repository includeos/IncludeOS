#include "serial.hpp"
#include <net/s2n/stream.hpp>
#include <string>
using s2n::print_s2n_error;
static s2n_config* config = nullptr;
static std::string stored_ca_cert = "";
static std::string stored_ca_key  = "";

// allow all clients
static uint8_t verify_host_passthrough(const char*, size_t, void* /*data*/) {
    return 1;
}

namespace s2n
{
void serial_test(
    const std::string& ca_cert,
    const std::string& ca_key)
{
#ifdef __includeos__
    setenv("S2N_DONT_MLOCK", "0", 1);
#endif
  setenv("S2N_ENABLE_CLIENT_MODE", "true", 1);
  if (s2n_init() < 0) {
    print_s2n_error("Error running s2n_init()");
    exit(1);
  }

  stored_ca_cert  = ca_cert;
  stored_ca_key   = ca_key;
}

s2n_config* serial_create_config()
{
  if (config) s2n_config_free(config);
  config = s2n_config_new();
  assert(config != nullptr);

  int res = s2n_config_add_cert_chain_and_key(config,
            stored_ca_cert.c_str(), stored_ca_key.c_str());
  if (res < 0) {
    print_s2n_error("Error getting certificate/key");
    exit(1);
  }

  res = s2n_config_disable_x509_verification(config);
  if (res < 0) {
    print_s2n_error("Error disabling x509 validation");
    exit(1);
  }

  res =
  s2n_config_set_verify_host_callback(config, verify_host_passthrough, nullptr);
  if (res < 0) {
    print_s2n_error("Error setting verify-host callback");
    exit(1);
  }
  
  return config;
}

s2n_config* serial_get_config()
{
  return config;
}

void serial_free_config()
{
  s2n_config_free(config);
  config = nullptr;
}

} // s2n
