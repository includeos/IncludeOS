

#ifndef MENDER_COMMON_HPP
#define MENDER_COMMON_HPP

#include <fs/disk.hpp>
#include <botan/secmem.h>

namespace mender {

  using Storage   = std::shared_ptr<fs::Disk>;

  /** Byte sequence */
  using byte_seq  = std::vector<uint8_t>;
  //using byte_seq = Botan::secure_vector<Botan::byte>;
}

#endif
