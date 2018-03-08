#include <system_log>
#include <kernel/os.hpp>

__attribute__((weak))
void SystemLog::write(const char* buffer, size_t length) {
  OS::print(buffer, length);
}

__attribute__((weak))
std::vector<char> SystemLog::copy() {
  return {/* override me */};
}

__attribute__((weak))
void SystemLog::initialize()
{
  /* override me */
}
