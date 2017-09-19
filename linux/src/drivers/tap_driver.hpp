#pragma once
#include <delegate>
#include <string>

struct TAP_driver
{
  typedef delegate<void(const void*, int)> on_read_func;
  void on_read(on_read_func func) { o_read = func; }
  void wait();

  int read (char *buf, int len);
  int write(const void* buf, int len);

  TAP_driver(const char* dev);
  ~TAP_driver();

private:
  int set_if_route(const char* cidr = "10.0.0.0/24");
  int set_if_address(const char* ip = "10.0.0.1");
  int set_if_up();
  int alloc_tun();

  int tun_fd;
  on_read_func o_read;
  const char* dev = nullptr;
};
