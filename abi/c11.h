#ifndef C11_H
#define C11_H

// More C11 requirements here
#include <quick_exit>

#ifdef __cplusplus
extern "C" {
#endif
  
  //New stuff in C11, required by libunwind, compiler-rt etc. in llvm
  
void *aligned_alloc( size_t alignment, size_t size );

#ifdef __cplusplus
}
#endif 

#endif //C11_H
