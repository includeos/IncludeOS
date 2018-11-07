#include <service>
#include <kernel/os.hpp>
#include <net/inet>
#include <hw/devices.hpp>
#include "drivers/tap_driver.hpp"
#include "drivers/usernet.hpp"
#include <vector>

static TAP_driver::TAPVEC tap_devices;

// create TAP device and hook up packet receive to UserNet driver
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

extern "C"
int userspace_main(int, char** args)
{
#ifdef __linux__
  // set affinity to CPU 1
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(1, &cpuset);
  sched_setaffinity(0, sizeof(cpuset), &cpuset);
#endif

  // initialize Linux platform
  OS::start(args[0]);

  // calls Service::start
  OS::post_start();
  return 0;
}

#ifndef LIBFUZZER_ENABLED
// default main (event loop forever)
int main(int argc, char** args)
{
  int res = userspace_main(argc, args);
  if (res < 0) return res;
  // begin event loop
  OS::event_loop();
  printf("*** System shutting down!\n");
  return 0;
}
#endif

void wait_tap_devices()
{
  TAP_driver::wait(tap_devices);
}
