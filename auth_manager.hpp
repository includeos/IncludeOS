
#ifndef MENDER_AUTH_MANAGER_HPP
#define MENDER_AUTH_MANAGER_HPP

#include "auth.hpp"
#include "keystore.hpp"
#include <delegate>

namespace mender {

  class Auth_manager {
  private:
    using Request         = Auth_request;
    using Request_data    = Auth_request_data;

  public:

    Auth_manager(Keystore& ks, Dev_id id, int64_t seq = 0);

    Auth_request make_auth_request();

    Auth_token auth_token() const
    { return tenant_token_; }

    void set_auth_token(byte_seq token)
    { tenant_token_ = std::move(token); }

    Keystore& keystore()
    { return keystore_; }

  private:
    Keystore&       keystore_;
    const Dev_id    id_data_;
    Auth_token      tenant_token_;
    int64_t         seq_;

  };

} // < namespace mender

#endif

