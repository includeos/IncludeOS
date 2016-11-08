#pragma once
#include <cstdint>

#define IRC_SERVER_VERSION    "v0.1"

#define NO_SUCH_CLIENT    UINT16_MAX
#define NO_SUCH_CHANNEL   UINT16_MAX

typedef uint32_t clindex_t;
typedef uint16_t chindex_t;

#include <hw/serial.hpp>
#include <cstring>
#include <cstdarg>

/**
 * The earliest possible print function (requires no heap, global ctors etc.)
 **/
inline void kprintf(const char* format, ...) {
  int bufsize = strlen(format) * 2;
  char buf[bufsize];
  va_list aptr;
  va_start(aptr, format);
  vsnprintf(buf, bufsize, format, aptr);
  hw::Serial::print1(buf);
}
