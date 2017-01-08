#pragma once
#include <cstdint>

#define IRC_SERVER_VERSION    "v0.2"

#define NO_SUCH_CLIENT         UINT32_MAX
#define NO_SUCH_CHANNEL        UINT16_MAX
#define NO_SUCH_CHANNEL_INDEX  UINT32_MAX
#define NO_SUCH_SERVER         UINT8_MAX

typedef uint32_t clindex_t;
typedef uint16_t chindex_t;
typedef uint8_t  sindex_t;
