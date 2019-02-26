#include <kernel/os.hpp>
#ifndef USERSPACE_LINUX
#include <version.h>
#endif
const char* OS::version_str_ = OS_VERSION;
const char* OS::arch_str_    = ARCH;
