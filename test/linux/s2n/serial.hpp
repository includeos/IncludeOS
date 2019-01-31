#include <net/s2n/stream.hpp>

namespace s2n
{
  using Stream_ptr = std::unique_ptr<TLS_stream>;
  
  extern void        serial_test(const std::string&, const std::string&);
  extern s2n_config* serial_create_config();
  extern s2n_config* serial_get_config();
  extern void        serial_free_config();
  
}
