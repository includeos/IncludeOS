// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 Oslo and Akershus University College of Applied Sciences
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
#ifndef INCLUDE_FD_COMPATIBLE_HPP
#define INCLUDE_FD_COMPATIBLE_HPP

#include <posix/fd.hpp>
#include <delegate>

/**
 * @brief      Makes classes inheriting this carry the
 *             attribute to be able to create a
 *             file descriptor of the resource.
 */
class FD_compatible {
public:
  delegate<FD&()> open_fd = nullptr;

  virtual ~FD_compatible() = default;

};

#endif
