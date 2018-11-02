#pragma once
#ifdef __MACH__
extern void* memalign(size_t alignment, size_t size);
#else
#include_next <malloc.h>
#endif
