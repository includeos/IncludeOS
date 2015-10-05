// Our patches / additions to newlibs partial implementation
#ifndef SYS_STDLIB_H
#define SYS_STDLIB_H

#include_next <stdlib.h>

// More C11 requirements here
#include "quick_exit"

#ifdef __cplusplus
extern "C" {
#endif
  
  //New stuff in C11, required by libunwind, compiler-rt etc. in llvm
  
void *aligned_alloc( size_t alignment, size_t size );

#ifdef __cplusplus
}
#endif 
#endif //SYS_STDLIB_H


