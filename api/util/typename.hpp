
#include <hal/machine.hpp>
#include <util/units.hpp>
#include <kernel/memory.hpp>
#include <string>

// Demangle
extern "C" char* __cxa_demangle(const char* mangled_name,
                            char*       output_buffer,
                            size_t*     length,
                            int*        status);

namespace os {
  using Machine_str = std::basic_string<char,
                                        std::char_traits<char>,
                                        os::Machine::Allocator<char>>;

  inline Machine_str demangle(const char* name) {
    using namespace util::literals;

    if (not mem::heap_ready() or name == nullptr) {
      return name;
    }

    int status = -1;
    size_t size = 1_KiB;

    auto str = Machine_str{};
    str.reserve(size);
    char* buf = str.data();
    buf =  __cxa_demangle(name, buf, &size, &status);

    if (UNLIKELY(status != 0)) {
      return {name};
    }

    return str;
  }
}
