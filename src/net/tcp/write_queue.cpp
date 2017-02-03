#include <net/tcp/write_queue.hpp>

using namespace net::tcp;

__attribute__((weak))
int WriteQueue::deserialize_from(void*) { return 0; }
__attribute__((weak))
int WriteQueue::serialize_to(void*) const { return 0; }
