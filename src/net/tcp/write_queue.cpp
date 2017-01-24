#include <net/tcp/write_queue.hpp>

using namespace net::tcp;

__attribute__((weak))
void WriteQueue::deserialize_from(void*) {}
__attribute__((weak))
int  WriteQueue::serialize_to(void*) { return 0; }
