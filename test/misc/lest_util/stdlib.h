#pragma once
#include <stddef.h>
#ifdef __MACH__
void* aligned_alloc(size_t alignment, size_t size);
#endif

#include_next <stdlib.h>
