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
#ifndef INCLUDE_FD_MAP_HPP
#define INCLUDE_FD_MAP_HPP

#include "fd.hpp"
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

class FD_map_error : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class FD_not_found : public FD_map_error {
public:
  FD_not_found(int id)
    : FD_map_error(std::string{"ID "} + std::to_string(id) + " not found")
  {}
};

/**
 * @brief File descriptor map
 * @details Singleton class that manages all the file descriptors
 */
class FD_map {
public:
  using id_t = FD::id_t;

  static FD_map& instance()
  {
    static FD_map fd_map;
    return fd_map;
  }

  template <typename T, typename... Args>
  T& open(Args&&... args);

  FD& get(const id_t id) const;

  int close(const id_t id)
  { return (map_.erase(id)) ? 0 : -1; }

  template <typename T, typename... Args>
  static auto&& _open(Args&&... args)
  { return instance().open<T>(std::forward<Args>(args)...); }

  static auto&& _get(const id_t id)
  { return instance().get(id); }

  static int _close(id_t id)
  { return instance().close(id); }

private:
  std::map<id_t, std::unique_ptr<FD>> map_;

  FD_map() {}

  id_t& counter()
  {
    static id_t counter_ = 3;
    return counter_;
  }

};

template <typename T, typename... Args>
T& FD_map::open(Args&&... args)
{
  static_assert(std::is_base_of<FD, T>::value,
    "Template argument is not a File Descriptor (FD)");

  const auto id = counter()++;
  auto* fd = new T(id, std::forward<Args>(args)...);
  map_.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(fd));
  return *fd;
}

inline FD& FD_map::get(const id_t id) const
{
  auto it = map_.find(id);
  if(it != map_.end())
    return *it->second;
  throw FD_not_found{id};
}

#endif
