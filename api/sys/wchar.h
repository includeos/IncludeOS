#include_next <wchar.h>

#ifndef SYS_WCHAR_H
#define SYS_WCHAR_H

#ifdef __cplusplus
extern "C" {
#endif

/** @WARNING Loses precision **/
static inline long double wcstold (const wchar_t* str, wchar_t** endptr) {
  return wcstod(str,endptr);
}

#ifdef __cplusplus
}
#endif

#endif // SYS_WCHAR_H
