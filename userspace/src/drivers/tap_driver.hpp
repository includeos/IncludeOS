#pragma once
#include <delegate>
#include <string>
#include <vector>
#include <sys/epoll.h>

struct TAP_driver
{
  typedef delegate<void(const void*, int)> on_read_func;
  TAP_driver(const char* dev, const char* ip);
  ~TAP_driver();

  void give_payload(const char* buffer, int len) {
    if (m_on_read) m_on_read(buffer, len);
  }

  void on_read(on_read_func func) { this->m_on_read = std::move(func); }
  int get_fd() const { return tun_fd; }
  int read (char *buf, int len);
  int write(const void* buf, int len);

  int bridge_add_if(const std::string& bridge);
private:
  int set_if_up();
  int set_if_route(const char* cidr);
  int set_if_address(const char* ip);
  int alloc_tun();

  int tun_fd;
  std::string  m_dev;
  on_read_func m_on_read = nullptr;
  std::unique_ptr<epoll_event> m_epoll = nullptr;
};
