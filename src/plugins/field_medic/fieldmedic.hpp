
/**
   Field medic - OS diagnostic plugins
   ____n_
   | +  |_\-;
   ;@-----@-'

   Adds extra health checks that will affect boot time and memory usage,
   but should add little or no performance overhead after service start.
**/

#include <array>
#include <stdexcept>

namespace medic{
  namespace diag
  {

    void init_tls();
    bool timers();
    bool elf();
    bool virtmem();
    bool tls();
    bool exceptions();
    bool stack();

    class Error : public std::runtime_error {
    public:
      using std::runtime_error::runtime_error;
      Error()
        : std::runtime_error("This is not a drill")
      {  /* TODO: Verify TLS */ }
    };


    const int bufsize = 1024;
    using Tl_bss_arr  = std::array<char, bufsize>;
    using Tl_data_arr = std::array<int, 256>;

  }
}
