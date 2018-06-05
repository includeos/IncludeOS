#include <net/tcp/stream.hpp>
#include <net/tcp/tcp.hpp>

namespace net::tcp
{
  /*#include <iostream> // remove me, sack debugging
  std::ostream& operator<< (std::ostream& out, const net::tcp::sack::Entries& ent) {
    for (auto el : ent) {
      out << el << "\n";
    }
    return out;
  }*/

  int Stream::get_cpuid() const noexcept {
    return m_tcp->host().get_cpuid();
  }
  
}
