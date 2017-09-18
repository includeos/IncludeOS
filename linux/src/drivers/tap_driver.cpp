#include "tap_driver.hpp"
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdarg.h>

static bool debug = true;

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

TAP_driver::TAP_driver(const char* devname)
{
  this->dev = devname;
  this->tun_fd = alloc_tun();

  if (set_if_up() != 0) {
      printf("[TAP] ERROR when setting up if\n");
      std::abort();
  }

  if (set_if_route() != 0) {
      printf("[TAP] ERROR when setting route for if\n");
      std::abort();
  }

  if (set_if_address() != 0) {
      printf("[TAP] ERROR when setting addr for if\n");
      std::abort();
  }
}

TAP_driver::~TAP_driver()
{
  close (tun_fd);
}

void TAP_driver::wait()
{
  fd_set readset;
  int    result;
  do {
     FD_ZERO(&readset);
     FD_SET(tun_fd, &readset);
     result = select(tun_fd + 1, &readset, NULL, NULL, NULL);
  } while (result == -1 && errno == EINTR);

  if (result > 0) {
     if (FD_ISSET(tun_fd, &readset)) {
       char buffer[1600];
       int len = this->read(buffer, sizeof(buffer));
       // on_read callback
       if (o_read) o_read(buffer, len);
     }
  }
  else if (result < 0) {
     /* An error ocurred, just print it to stdout */
     printf("Error on select(): %s\n", strerror(errno));
  }
}
