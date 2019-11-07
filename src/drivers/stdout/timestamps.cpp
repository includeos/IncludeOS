
#include <os.hpp>

__attribute__((constructor))
static void enable_timestamps()
{
  os::print_timestamps(true);
}
