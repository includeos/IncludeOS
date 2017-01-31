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

class SHA1
{
public:
    static const size_t BLOCK_INTS = 16;  /* number of 32bit integers per SHA1 block */
    static const size_t BLOCK_BYTES = BLOCK_INTS * 4;

    SHA1();

    void update(const std::string&);
    void update(const char*, size_t);
    std::string final();

    static std::string oneshot(const std::string&);

private:
    uint64_t transforms;
    uint32_t digest[5];
    uint32_t buffer_len = 0;
    char     buffer[BLOCK_BYTES+2];
};


#endif /* SHA1_HPP */
