#include <net/s2n/stream.hpp>
#include "liveupdate.hpp"
#include "storage.hpp"

namespace liu
{
  net::Stream_ptr Restore::as_tls_stream(void* vctx, bool outgoing,
                                         net::Stream_ptr transport)
  {
    if (ent->type != TYPE_STREAM) {
      throw std::runtime_error("LiveUpdate: Restore::as_tls_stream() encountered incorrect type " + std::to_string(ent->type));
    }
    if (ent->id != s2n::TLS_stream::SUBID) {
      throw std::runtime_error("LiveUpdate: Restore::as_tls_stream() encountered incorrect subid " + std::to_string(ent->id));
    }
    auto* config = (s2n_config*) vctx;
    return s2n::TLS_stream::deserialize_from(
                    config,
                    std::move(transport),
                    outgoing,
                    ent->data(), ent->len
                  );
  }
}
