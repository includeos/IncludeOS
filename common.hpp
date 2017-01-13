#pragma once
#include <cstdint>

#define IRC_SERVER_VERSION    "v0.2"

#define READQ_MAX             512

typedef uint32_t clindex_t;
typedef uint16_t chindex_t;
typedef int      sindex_t;

#define NO_SUCH_CLIENT         ((clindex_t) -1)
#define NO_SUCH_CHANNEL        ((chindex_t) -1)
#define NO_SUCH_CHANNEL_INDEX  UINT32_MAX
#define NO_SUCH_SERVER         ((int) -1)
