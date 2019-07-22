
#include <common.cxx>
#include <util/crc32.hpp>


CASE("Various text strings with known CRC32 matches")
{
  std::string q1 = "... or paste text here ......f";
  uint32_t    a1 = 0x87bbc094;
  
  std::string q2 = "This is a really really really really really really "
                   "really really really really really really really "
                   "really really really really really really really "
                   "really really really really really really really "
                   "really really really really really really really "
                   "really really really really long string.";
  uint32_t    a2 = 0x2eb5c684;
  
  EXPECT(crc32(q1.c_str(), q1.size()) == a1);
  EXPECT(crc32(q2.c_str(), q2.size()) == a2);
  
  // Intel CRC32-C
  EXPECT(crc32_fast(q1.c_str(), q1.size()) == crc32c(q1.c_str(), q1.size()));
  EXPECT(crc32_fast(q2.c_str(), q2.size()) == crc32c(q2.c_str(), q2.size()));
  
}
