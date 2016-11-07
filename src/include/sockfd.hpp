// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#ifndef INCLUDE_SOCKFD_HPP
#define INCLUDE_SOCKFD_HPP

#include <sys/socket.h>
#include <net/inet4>
#include "fd.hpp"

class SockFD : public FD {
public:
  explicit SockFD(const int id)
      : FD(id)
  {}
  
  bool is_socket() override { return true; }
  
  typedef delegate<void(net::tcp::buffer_t, size_t)> on_read_func;
  typedef delegate<void()> on_write_func;
  typedef delegate<void()> on_except_func;
  
  virtual on_read_func   get_default_read_func()   = 0;
  virtual on_write_func  get_default_write_func()  = 0;
  virtual on_except_func get_default_except_func() = 0;
};

#endif
