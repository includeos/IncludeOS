#include <service>
#include <kernel/os.hpp>
#include <kernel/timers.hpp>
#include <net/inet>
#include <hw/devices.hpp>
#include "drivers/tap_driver.hpp"
#include "drivers/usernet.hpp"
#include <vector>

static std::vector<std::shared_ptr<TAP_driver>> tap_devices;

// create TAP device and hook up packet receive to UserNet driver
void create_network_device(int N, const char* ip)
{
  const std::string name = "tap" + std::to_string(N);
  auto tap = std::make_shared<TAP_driver> (name.c_str(), ip);
  tap_devices.push_back(tap);
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

namespace linux
{
  static int epoll_init_if_needed()
  {
    static int epoll_fd = -1;
    if (epoll_fd == -1) {
      if ((epoll_fd = epoll_create(1)) < 0)
      {
        fprintf(stderr, "ERROR when creating epoll fd\n");
        std::abort();
      }
    }
    return epoll_fd;
  }
  void epoll_add_fd(int fd, epoll_event& event)
  {
    const int efd = epoll_init_if_needed();
    // register event to epoll instance
    if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event) < 0)
    {
      fprintf(stderr, "ERROR when adding fd to epoll\n");
      std::abort();
    }
  }
  void epoll_del_fd(int fd)
  {
    const int efd = epoll_init_if_needed();
    // unregister event to epoll instance
    if (epoll_ctl(efd, EPOLL_CTL_DEL, fd, nullptr) < 0)
    {
      fprintf(stderr, "ERROR when removing fd from epoll\n");
      std::abort();
    }
  }
  void epoll_wait_events()
  {
    // get timeout from time to next timer in timer system
    // NOTE: when next is 0, it means there is no next timer
    const unsigned long long next = Timers::next().count();
    int timeout = (next == 0) ? -1 : (1 + next / 1000000ull);
  
    if (timeout < 0 && tap_devices.empty()) {
      printf("epoll_wait_events(): Deadlock reached\n");
      std::abort();
    }
  
    const int efd = epoll_init_if_needed();
    std::array<epoll_event, 16> events;
    //printf("epoll_wait(%d milliseconds) next=%llu\n", timeout, next);
    int ready = epoll_wait(efd, events.data(), events.size(), timeout);
    if (ready < 0) {
      // ignore interruption from signals
      if (errno == EINTR) return;
      printf("[TAP] ERROR when waiting for epoll event\n");
      std::abort();
    }
    for (int i = 0; i < ready; i++)
    {
      for (auto& tap : tap_devices)
      {
        const int fd = events.at(i).data.fd;
        if (tap->get_fd() == fd)
        {
          char buffer[9000];
          int len = tap->read(buffer, sizeof(buffer));
          // hand payload to driver
          tap->give_payload(buffer, len);
          break;
        } // tap devices
      }
    }
  } // epoll_wait_events()
}
