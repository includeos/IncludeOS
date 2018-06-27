/*
    sha1.hpp - header of

    ============
    SHA-1 in C++
    ============

    100% Public Domain.

    Original C Code
        -- Steve Reid <steve@edmweb.com>
    Small changes to fit into bglibs
        -- Bruce Guenter <bruce@untroubled.org>
    Translation to simpler C++ Code
        -- Volker Grabsch <vog@notjusthosting.com>
    Safety fixes
        -- Eugene Hopkinson <slowriot at voxelstorm dot com>
    Remove streams, perf improvements, port to IncludeOS
        -- fwsGonzo
*/

#ifndef UTIL_SHA1_HPP
#define UTIL_SHA1_HPP

#include <cstdint>
#include <string>
#include <vector>

class SHA1
{
public:
    static const size_t BLOCK_INTS = 16;  /* number of 32bit integers per SHA1 block */
    static const size_t BLOCK_BYTES = BLOCK_INTS * 4;

    SHA1();
    // update with new data
    void update(const std::string&);
    void update(const std::vector<char>&);
    void update(const void*, size_t);
    // finalize values
    std::vector<char> as_raw();  // 20 bytes
    std::string       as_hex();  // 40 bytes

    // 20 byte SHA1 raw value
    static std::vector<char> oneshot_raw(const std::vector<char>&);
    // 40 byte SHA1 hex string
    static std::string oneshot_hex(const std::string&);

private:
    void finalize();
    uint64_t transforms;
    uint32_t digest[5];
    uint32_t buffer_len = 0;
    char     buffer[BLOCK_BYTES+2];
};


#endif /* SHA1_HPP */
