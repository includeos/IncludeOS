
#ifndef KERNEL_CRASH_CONTEXT_HPP
#define KERNEL_CRASH_CONTEXT_HPP

#include <sys/types.h>

char*  get_crash_context_buffer();
size_t get_crash_context_length();

#ifndef SET_CRASH_CONTEXT
// used to set a message that will be printed on crash the message is to
// be contextual helping to identify the reason for crashes
// Example: copy HTTP requests into buffer during stress or malformed request
// testing if server crashes we can inspect the HTTP request to identify which
// one caused the crash
  #define SET_CRASH_CONTEXT(X,...)  snprintf( \
          get_crash_context_buffer(), get_crash_context_length(), \
          X, ##__VA_ARGS__);
#else
  #define SET_CRASH_CONTEXT(X,...)  /* */
#endif

#ifndef DISABLE_CRASH_CONTEXT
#define SET_CRASH SET_CRASH_CONTEXT
#else
#define SET_CRASH(...) /* */
#endif

#endif //< KERNEL_CRASH_CONTEXT_HPP
