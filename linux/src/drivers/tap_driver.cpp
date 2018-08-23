#include "tap_driver.hpp"
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <stdarg.h>

static bool debug = false;

static int run_cmd(const char *cmd, ...)
{
  static const int CMDBUFLEN = 512;

  va_list ap;
  char buf[CMDBUFLEN];
  va_start(ap, cmd);
  vsnprintf(buf, CMDBUFLEN, cmd, ap);
  va_end(ap);

  if (debug) {
      printf("EXEC: %s\n", buf);
  }

  return system(buf);
}

int TAP_driver::set_if_route(const char *cidr)
{
  return run_cmd("ip route add dev %s %s", dev, cidr);
}

int TAP_driver::set_if_address(const char* ip)
{
  return run_cmd("ip address add dev %s local %s", dev, ip);
}

int TAP_driver::set_if_up()
{
  return run_cmd("ip link set dev %s up", dev);
}

/*
 * Taken from Kernel Documentation/networking/tuntap.txt
 */
int TAP_driver::alloc_tun()
{
  assert(this->dev != nullptr);
  struct ifreq ifr;
  int fd, err;

  if ((fd = open("/dev/net/tap", O_RDWR)) < 0) {
      perror("Cannot open TUN/TAP dev\n"
                  "Make sure one exists with "
                  "'$ mknod /dev/net/tap c 10 200'");
      exit(1);
  }

  /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
   *        IFF_TAP   - TAP device
   *
   *        IFF_NO_PI - Do not provide packet information
   */
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
  strncpy(ifr.ifr_name, dev, IFNAMSIZ);

  if ((err = ioctl(fd, (int) TUNSETIFF, (void *) &ifr)) < 0) {
      perror("ERR: Could not ioctl tun");
      close(fd);
      return err;
  }

  dev = strdup(ifr.ifr_name);
  return fd;
}

int TAP_driver::read(char *buf, int len)
{
  return ::read(tun_fd, buf, len);
}

int TAP_driver::write(const void* buf, int len)
{
  return ::write(tun_fd, buf, len);
}

TAP_driver::TAP_driver(const char* devname,
                       const char* route,
                       const char* ip)
{
  this->dev = devname;
  this->tun_fd = alloc_tun();

  if (set_if_up() != 0) {
      printf("[TAP] ERROR when setting up if\n");
      std::abort();
  }

  if (route != nullptr)
  if (set_if_route(route) != 0) {
      printf("[TAP] ERROR when setting route for if\n");
      std::abort();
  }

  if (set_if_address(ip) != 0) {
      printf("[TAP] ERROR when setting addr for if\n");
      std::abort();
  }

  // setup epoll() functionality
  if ((this->epoll_fd = epoll_create(1)) < 0)
  {
    printf("[TAP] ERROR when creating epoll fd\n");
    std::abort();
  }

  epoll_ptr = new epoll_event;
  memset(epoll_ptr, 0, sizeof(epoll_event));
  epoll_ptr->events = EPOLLIN;
  epoll_ptr->data.fd = this->tun_fd;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, this->tun_fd, epoll_ptr) < 0)
  {
    printf("[TAP] ERROR when adding tap to epoll event\n");
    std::abort();
  }
}

TAP_driver::~TAP_driver()
{
  delete epoll_ptr;
  close (this->tun_fd);
  close (this->epoll_fd);
}

void TAP_driver::wait()
{
  const int MAX_EVENTS = 1;
  int ready = epoll_wait(this->epoll_fd, this->epoll_ptr, MAX_EVENTS, -1);
  if (ready < 0) {
    if (errno == EINTR) return;
    printf("[TAP] ERROR when waiting for epoll event\n");
    std::abort();
  }
  if (epoll_ptr->events & EPOLLIN)
  {
    char buffer[1600];
    int len = this->read(buffer, sizeof(buffer));
    // on_read callback
    this->on_read(buffer, len);
  }
}

void TAP_driver::wait(TAPVEC& tap_devices)
{
  for (auto& tapref : tap_devices)
  {
    auto& tapdev = tapref.get();
    tapdev.wait();
  }
}
