
#pragma once
#ifndef INCLUDE_SOCKFD_HPP
#define INCLUDE_SOCKFD_HPP

#include <sys/socket.h>
#include <net/inet>
#include "fd.hpp"

class SockFD : public FD {
public:
  explicit SockFD(const int id)
      : FD(id)
  {}

  bool is_socket() override { return true; }
};

struct sockaddr;
typedef uint32_t socklen_t;
extern bool validate_sockaddr_in(const struct sockaddr*, socklen_t);

#endif
