
#pragma once
#ifndef NET_CHECKSUM_HPP
#define NET_CHECKSUM_HPP

#include <cstdint>
#include <cstddef>

namespace net {

  // Compute internet checksum with partial @sum provided
  uint16_t checksum(uint32_t sum, const void* data, size_t len) noexcept;

  // Compute the internet checksum for the buffer / buffer part provided
  inline uint16_t checksum(const void* data, size_t len) noexcept {
    return checksum(0, data, len);
  }

  /**
   * @brief      Adjust the checksum according to the difference between old and new data.
   *
   * @note       Only supports even offsets (length needs to be even)
   *             See: https://tools.ietf.org/html/rfc3022#page-9 (4.2) Checksum Adjustment
   *
   * @param      chksum  Pointer to the checksum to adjust
   * @param      odata   The old data
   * @param[in]  olen    The length of the old data
   * @param      ndata   The new data
   * @param[in]  nlen    The length of the new data
   */
  void checksum_adjust(uint8_t* chksum, const void* odata,
                       int olen, const void* ndata, int nlen);

  /**
   * @brief      Helper function for adjusting checksum when only an object is changed,
   *             e.g. IP address in a packet header
   *
   * @param      chksum   Pointer to the checksum to adjust
   * @param[in]  old_obj  The old object
   * @param[in]  new_obj  The new object
   *
   * @tparam     T        The type of object
   */
  template <typename T>
  void checksum_adjust(uint16_t* chksum, const T* old_obj, const T* new_obj)
  {
    static_assert(sizeof(T) % 2 == 0, "Checksum adjust only supports even lengths");
    checksum_adjust(reinterpret_cast<uint8_t*>(chksum), old_obj, sizeof(T), new_obj, sizeof(T));
  }

} //< namespace net

#endif //< NET_CHECKSUM_HPP
