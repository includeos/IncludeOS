
#include <os.hpp>
#ifndef USERSPACE_KERNEL
#include <version.h>
#endif

const char* os::version() noexcept {
  return OS_VERSION;
}
