#include <service>
#include <kernel/os.hpp>
#include <net/inet4.hpp>
#include <hw/devices.hpp>
#include "drivers/tap_driver.hpp"
#include "drivers/usernet.hpp"

// create TAP device and hook up packet receive to UserNet driver
static TAP_driver tap0("tap1");
static void packet_sent(net::Packet_ptr packet);
extern void __platform_init();
static std::unique_ptr<net::Inet4> network = nullptr;

int main(void)
{
  __platform_init();

  // the network userspace driver
  auto* usernet = new UserNet();
  usernet->set_transmit_forward(packet_sent);
  // register driver
  auto driver = std::unique_ptr<hw::Nic> (usernet);
  hw::Devices::register_device<hw::Nic> (std::move(driver));
  // connect userspace driver to tap device
  tap0.on_read({usernet, &UserNet::write});

  // configure default network stack
  auto& network = net::Super_stack::get<net::IP4> (0);
  network.network_config(
    { 10,  0,  0,  2},
    {255,255,255,  0},
    { 10,  0,  0,  1}, // GW
    {  8,  8,  8,  8}  // DNS
  );

  // calls Service::start
  OS::post_start();

  // begin event loop
  OS::event_loop();
  printf("*** System shutting down!\n");
  return 0;
}

// send packet to Linux
void packet_sent(net::Packet_ptr packet)
{
  //printf("write %d\n", packet->size());
  tap0.write(packet->layer_begin(), packet->size());
}

void wait_tap_devices()
{
  tap0.wait();
}
