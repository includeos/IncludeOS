#pragma once
#include <delegate>
#include <functional>
#include <string>
#include <vector>

struct epoll_event;

struct TAP_driver
{
  typedef std::vector<std::reference_wrapper<TAP_driver>> TAPVEC;
  typedef delegate<void(const void*, int)> on_read_func;
  void on_read(on_read_func func) { o_read = func; }
  static void wait(TAPVEC&);

  void on_read(const char* buffer, int len) {
    if (o_read) o_read(buffer, len);
  }

  void wait();

  int get_fd() const { return tun_fd; }
  int read (char *buf, int len);
  int write(const void* buf, int len);

  TAP_driver(const char* dev, const char* cidr, const char* ip);
  ~TAP_driver();

private:
  int set_if_up();
  int set_if_route(const char* cidr);
  int set_if_address(const char* ip);
  int alloc_tun();

  int tun_fd;
  on_read_func o_read = nullptr;
  const char* dev = nullptr;

  int epoll_fd = -1;
  epoll_event* epoll_ptr = nullptr;
};
