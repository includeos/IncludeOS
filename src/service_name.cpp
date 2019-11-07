
#include <service>
extern "C" {
    __attribute__((noreturn)) void panic(const char* reason);
}

#ifndef __linux__
extern "C" __attribute__((noreturn))
void abort(){
    panic("Abort called");
    __builtin_unreachable();
}
#endif

const char* service_name__ = SERVICE_NAME;
const char* service_binary_name__ = SERVICE;

namespace os {
    uintptr_t liveupdate_memory_size_mb = _LIVEUPDATE_MEMSIZE_;
}
