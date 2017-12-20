#include <service>
#include <kernel/os.hpp>
#include <net/inet4.hpp>
#include <hw/devices.hpp>
#include "drivers/tap_driver.hpp"
#include "drivers/usernet.hpp"
#include <vector>

// create TAP device and hook up packet receive to UserNet driver
extern void __platform_init();
static TAP_driver::TAPVEC tap_devices;

void create_network_device(int N, const char* route, const char* ip)
{
  auto tap = std::make_shared<TAP_driver> (
            ("tap" + std::to_string(N)).c_str(), route, ip);
  tap_devices.push_back(*tap);
  // the IncludeOS packet communicator
  auto* usernet = new UserNet(1500);
  // register driver for superstack
  auto driver = std::unique_ptr<hw::Nic> (usernet);
  hw::Devices::register_device<hw::Nic> (std::move(driver));
  // connect driver to tap device
  usernet->set_transmit_forward(
    [tap] (net::Packet_ptr packet) {
      tap->write(packet->layer_begin(), packet->size());
    });
  tap->on_read({usernet, &UserNet::receive});
}

int main(void)
{
  __platform_init();

  // calls Service::start
  OS::post_start();

  // begin event loop
  OS::event_loop();
  printf("*** System shutting down!\n");
  return 0;
}

void wait_tap_devices()
{
  TAP_driver::wait(tap_devices);
}
