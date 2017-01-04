
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

  Auth_manager::Auth_manager(Keystore& ks, Dev_id id, int64_t seq)
    : keystore_(ks), id_data_(std::move(id)), seq_(seq)
  {
    std::string tkn{""};
    tenant_token_ = Auth_token{tkn.begin(), tkn.end()};
  }

  Auth_request Auth_manager::make_auth_request()
  {
    Request_data authd;

    // Populate request data
    authd.id_data       = this->id_data_;
    authd.pubkey        = this->keystore_.public_PEM();
    authd.tenant_token  = this->tenant_token_;
    authd.seq_no        = this->seq_++;

    printf("<Auth_manager> Next seqno=%lli\n", seq_);
    // Get serialized bytes
    const auto reqdata = authd.serialized_bytes();

    // Generate signature from payload
    const auto signature = keystore_.sign(reqdata);

    return Request {
      .data       = reqdata,
      .token      = this->tenant_token_,
      .signature  = signature
    };
  }

} // < namespace mender

#endif

