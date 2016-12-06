

#ifndef MENDER_COMMON_HPP
#define MENDER_COMMON_HPP

#include <botan/secmem.h>

namespace mender {

  /** Byte sequence */
  //using byte_seq = Botan::secure_vector<Botan::byte>;
  using byte_seq = std::vector<uint8_t>;
}

#endif
