#include <os>
#include <assert.h>

// mocked commandline arguments
const std::string& OS::cmdline_args() noexcept
{
  static const std::string args("binary.bin command line arguments passed here");
  return args;
}

extern "C" {

  void panic(const char* why){

    assert(0 && "Panic: " && why);
  }

}
