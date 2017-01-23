#include <sys/select.h>
#include <errno.h>
#include <stdio.h>
#include <fd_map.hpp>
#include <tcp_fd.hpp>
#include <list>

static struct {
  typedef std::pair<int, TCP_FD&> listpair;
  std::list<listpair>                 list;
  std::list<net::tcp::Connection_ptr> conn;
  
  void clean() {
    // reset callbacks on all listeners
    for (auto& l : list) {
      l.second.get_listener().on_connect([] (auto) {});
    }
    list.clear();
    // reset callbacks on all connections
    for (auto c : conn) {
      c->setup_default_callbacks();
    }
    conn.clear();
  }
  
} temp;

static int process_fds(
    int      max_fd,
    fd_mask* bits,
    delegate<int(int)> callback)
{
  for (size_t i = 0; i < sizeof (fd_set) / sizeof(fd_mask); i++)
  {
    int fd = sizeof(fd_mask) * i * 8;
    fd_mask mask = bits[i];
    while (mask)
    {
      int lbits = __builtin_ctzl(mask);
      fd += lbits;
      // process descriptor
      int ret = callback(fd);
      if (ret) return ret;
      
      mask >>= lbits + 1;
      if (++fd >= max_fd) return 0;
    }
    if (fd >= max_fd) return 0;
  }
  return 0;
}

int  select(int max_fd,
            fd_set *__restrict__ reads,
            fd_set *__restrict__ writes,
            fd_set *__restrict__ excepts,
            struct timeval *__restrict__ tv)
{
  // monitor file descriptors given in read, write and exceptional fd sets
  int mon_read = -1;
  int mon_send = -1;
  int mon_err  = -1;
  int timer    = -1;
  bool timeout = false;
  int ret = 0;
  
  if (reads)
  {
    ret = process_fds(max_fd, reads->fds_bits,
    [] (int fd) -> int {
        
        try {
          auto& desc = FD_map::_get(fd);
          if (desc.is_socket()) {
            auto& tcp = (TCP_FD&) desc;
            if (tcp.is_listener()) {
              /// monitor for new connections
              temp.list.push_back({fd, tcp});
            }
            else if (tcp.is_connection()) {
              /// monitor for reads
            }
          }
          return 0;
          
        } catch (const FD_not_found&) {
          errno = EBADF;
          return -1;
        }
      
    });
    if (ret) return ret;
    // clear read fds
    FD_ZERO(reads);
  }
  if (writes)
  {
    ret = process_fds(max_fd, writes->fds_bits,
    [&mon_send] (int) -> int {
      errno = ENOTSUP;
      return -1;
    });
    if (ret) return ret;
    // clear send fds
    FD_ZERO(writes);
  }
  if (excepts)
  {
    ret = process_fds(max_fd, excepts->fds_bits,
    [&mon_err] (int) -> int {
      errno = ENOTSUP;
      return -1;
    });
    if (ret) return ret;
    // clear exception fds
    FD_ZERO(excepts);
  }
  // timeout
  int64_t micros = tv->tv_sec * 1000000 + tv->tv_usec;
  if (micros > 0) {
    timer = Timers::oneshot(
      std::chrono::microseconds(micros),
      [&timeout] (auto) {
        timeout = true;
      });
  }
  /// wait for something to happen
  do {
    for (auto& list : temp.list) {
      if (list.second.has_connq()) {
        mon_read = list.first; // fd
        break;
      }
    }
    
    OS::block();
  } while (mon_read == -1 
        && mon_send == -1 
        && mon_err  == -1 
        && timeout  == false);
  
  if (mon_read >= 0) FD_SET(mon_read, reads);
  if (mon_send >= 0) FD_SET(mon_send, writes);
  if (mon_err  >= 0) FD_SET(mon_err,  excepts);
  if (timer    >= 0) {
    if (timeout == false) Timers::stop(timer);
  }
  // clear all the temp stuff
  temp.clean();
  // all good
  return 0;
}
int  pselect(int max_fd,
             fd_set *__restrict__ reads,
             fd_set *__restrict__ writes,
             fd_set *__restrict__ excepts,
             const struct timespec *__restrict__ ts,
             const sigset_t *__restrict__)
{
  struct timeval tv;
  tv.tv_sec  = ts->tv_sec;
  tv.tv_usec = ts->tv_nsec / 1000;
  return select(max_fd, reads, writes, excepts, &tv);
}
