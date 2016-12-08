#include <cstdint>
#include <cstddef>

#define CRC32_BEGIN(x)   uint32_t x = 0xFFFFFFFF;
#define CRC32_VALUE(x)   ~(x)

uint32_t crc32(uint32_t partial, const char* buf, size_t len);

inline uint32_t crc32(const char* buf, size_t len)
{
  return ~crc32(0xFFFFFFFF, buf, len);
}
