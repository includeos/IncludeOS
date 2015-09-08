#ifndef C11_H
#define C11_H

#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
  
  //New stuff in C11, required by libunwind, compiler-rt etc. in llvm
  
int at_quick_exit (void (*func)(void));
_Noreturn void quick_exit (int status);

void *aligned_alloc( size_t alignment, size_t size );

#ifdef __cplusplus
}
#endif 

#endif //C11_H
