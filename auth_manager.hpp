
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

    Auth_manager(std::unique_ptr<Keystore> ks, Dev_id id, uint64_t seq = 0);

    Auth_request make_auth_request();

    Auth_token auth_token() const
    { return tenant_token_; }

    void set_auth_token(byte_seq token)
    { tenant_token_ = std::move(token); }

    Keystore& keystore()
    { return *keystore_; }

    auto seqno() const
    { return seq_; }

    void set_seqno(uint64_t s)
    { seq_ = s; }

  private:
    std::unique_ptr<Keystore>   keystore_;
    const Dev_id                id_data_;
    uint64_t                    seq_;
    Auth_token                  tenant_token_;

  };

} // < namespace mender

#endif

