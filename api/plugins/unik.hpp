
#ifndef PLUGINS_UNIK_HPP
#define PLUGINS_UNIK_HPP

#include <net/inet>

namespace unik{

  const net::UDP::port_t default_port = 9967;

  class Client {
  public:
    using Registered_event = delegate<void()>;

    static void register_instance(net::Inet& inet, const net::UDP::port_t port = default_port);
    static void register_instance_dhcp();
    static void on_registered(Registered_event e) {
      on_registered_ = e;
    };

  private:
    static Registered_event on_registered_;
  };

}

#endif
