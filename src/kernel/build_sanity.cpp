#include <common>

#if !defined(__cplusplus)
#error "IncludeOS must be built with a C++ compiler"
#endif

#ifdef ARCH_X86
static_assert(sizeof(void*) == 4, "Pointer must match arch");
#endif
#ifdef ARCH_X64
static_assert(sizeof(void*) == 8, "Pointer must match arch");
#endif
