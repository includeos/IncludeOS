// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <net/https/s2n_server.hpp>
#include <net/s2n/stream.hpp>
using s2n::print_s2n_error;

// allow all clients
static uint8_t verify_host_passthrough(const char*, size_t, void* /*data*/) {
    return 1;
}

namespace http
{
  void S2N_server::initialize(
      const std::string& ca_cert,
      const std::string& ca_key)
  {
#ifdef __includeos__
    setenv("S2N_DONT_MLOCK", "0", 1);
#endif
    if (s2n_init() < 0) {
      print_s2n_error("Error running s2n_init()");
      exit(1);
    }
    
    this->m_config = s2n_config_new();
    assert(this->m_config != nullptr);
    auto* config = (s2n_config*) this->m_config;
    
    int res =
    s2n_config_add_cert_chain_and_key(config, ca_cert.c_str(), ca_key.c_str());
    if (res < 0) {
      print_s2n_error("Error getting certificate/key");
      exit(1);
    }
    
    res =
    s2n_config_set_verify_host_callback(config, verify_host_passthrough, nullptr);
    if (res < 0) {
      print_s2n_error("Error setting verify-host callback");
      exit(1);
    }
  }
  
  S2N_server::~S2N_server()
  {
    s2n_config_free((s2n_config*) this->m_config);
  }
  
  void S2N_server::bind(const uint16_t port)
  {
    tcp_.listen(port, {this, &S2N_server::on_connect});
    INFO("HTTPS Server", "Listening on port %u", port);
  }

  void S2N_server::on_connect(TCP_conn conn)
  {
    connect(
      std::make_unique<s2n::TLS_stream> (
        (s2n_config*) this->m_config,
        std::make_unique<net::tcp::Stream>(std::move(conn)))
    );
  }
  
}
