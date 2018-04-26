#include <net/openssl/tls_stream.hpp>
#include <openssl/ssl.h>
#include "liveupdate.hpp"
#include "storage.hpp"
#include <cassert>
using namespace net;
typedef net::tcp::Connection_ptr Connection_ptr;

namespace openssl
{
  size_t TLS_stream::serialize_to(void* loc) const
  {
    auto* session = SSL_get_session(this->m_ssl);
    uint8_t* temp = (uint8_t*) loc;
    const int length = i2d_SSL_SESSION(session, &temp);
    return length;
  }
}

/// public API ///
namespace liu
{
  void Storage::add_tls_stream(uid id, net::Stream& stream)
  {
    auto* tls = dynamic_cast<openssl::TLS_stream*> (&stream);
    hdr.add_struct(TYPE_TLS_STREAM, id,
    [tls] (char* location) -> int {
      // return size of all the serialized data
      return tls->serialize_to(location);
    });
  }
  net::Stream_ptr Restore::as_tls_stream(void* vctx, net::Stream_ptr transport)
  {
    auto* ctx = (SSL_CTX*) vctx;

    SSL* ssl = SSL_new(ctx);
    // TODO: deserialize BIOs
    BIO* rd = BIO_new(BIO_s_mem());
    BIO* wr = BIO_new(BIO_s_mem());
    assert(ERR_get_error() == 0 && "Initializing BIOs");

    SSL_set_bio(ssl, rd, wr);

    // TODO: deserialize SSL session
    SSL_SESSION* session = nullptr;
    auto* temp = static_cast<const uint8_t*> (this->data());
    auto* rval = d2i_SSL_SESSION(&session, &temp, this->length());
    assert(rval != nullptr);

    int res = SSL_CTX_add_session(ctx, session);
    assert(res == 1);
    SSL_set_session(ssl, session);

    return std::make_unique<openssl::TLS_stream> (std::move(transport), ssl, rd, wr);
  }
}
