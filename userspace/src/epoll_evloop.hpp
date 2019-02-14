#pragma once
#include <sys/epoll.h>

namespace linux
{
  void epoll_add_fd(int fd, epoll_event& event);
  void epoll_del_fd(int fd);
  void epoll_wait_events();
}
