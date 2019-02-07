#include <system_log>
#include <os.hpp>

__attribute__((weak))
void SystemLog::write(const char* buffer, size_t length) {
  os::print(buffer, length);
}

__attribute__((weak))
std::vector<char> SystemLog::copy() {
  return {/* override me */};
}
__attribute__((weak))
uint32_t SystemLog::get_flags() { return 0; }
__attribute__((weak))
void     SystemLog::set_flags(uint32_t) {}
__attribute__((weak))
void     SystemLog::clear_flags() {}

__attribute__((weak))
void SystemLog::initialize()
{
  /* override me */
}
