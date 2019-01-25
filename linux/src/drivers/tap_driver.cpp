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
#include "../epoll_evloop.hpp"

static constexpr bool debug = true;

static int run_cmd(const char *cmd, ...)
{
  static const int CMDBUFLEN = 512;

  va_list ap;
  char buf[CMDBUFLEN];
  va_start(ap, cmd);
  vsnprintf(buf, CMDBUFLEN, cmd, ap);
  va_end(ap);

  if constexpr (debug) {
      printf("EXEC: %s\n", buf);
  }

  return system(buf);
}

int TAP_driver::set_if_up()
{
  return run_cmd("ip link set dev %s up", m_dev.c_str());
}
int TAP_driver::set_if_address(const char* ip)
{
  return run_cmd("ip addr add %s dev %s", ip, m_dev.c_str());
}

int TAP_driver::set_if_route(const char* cidr)
{
  return run_cmd("ip route add dev %s %s", m_dev.c_str(), cidr);
}
int TAP_driver::bridge_add_if(const std::string& interface)
{
  return run_cmd("brctl addif %s %s", interface.c_str(), m_dev.c_str());
  //return run_cmd("ip link set %s master %s", m_dev.c_str(), interface.c_str());
}

/*
 * Taken from Kernel Documentation/networking/tuntap.txt
 */
int TAP_driver::alloc_tun()
{
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
  strncpy(ifr.ifr_name, this->m_dev.c_str(), IFNAMSIZ);

  if ((err = ioctl(fd, (int) TUNSETIFF, (void *) &ifr)) < 0) {
      perror("ERR: Could not ioctl tun");
      close(fd);
      return err;
  }

  this->m_dev = std::string{ifr.ifr_name};
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
                       const char* ip)
{
  assert(devname != nullptr);
  assert(ip != nullptr);
  this->m_dev    = std::string{devname};
  this->tun_fd = alloc_tun();

  if (set_if_up() != 0) {
      printf("[TAP] ERROR when setting up if\n");
      std::abort();
  }
  if (set_if_address(ip) != 0) {
      printf("[TAP] ERROR when setting addr for if\n");
      std::abort();
  }

  // create epoll event
  this->m_epoll = std::make_unique<epoll_event> ();
  m_epoll->events = EPOLLIN;
  m_epoll->data.fd = this->tun_fd;
  // register ourselves to epoll instance
  linux::epoll_add_fd(this->tun_fd, *m_epoll);
}

TAP_driver::~TAP_driver()
{
  linux::epoll_del_fd(this->tun_fd);
  close (this->tun_fd);
}
