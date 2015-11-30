#ifndef SYS_CTYPE_H
#define SYS_CTYPE_H

#ifdef __cplusplus
extern "C" {
#endif

  static inline int isascii(int c){
    return c >= 0 && c <= 0177;
  }

  
#ifdef __cplusplus
}
#endif

#endif //SYS_CTYPE_H


#include_next <ctype.h>
