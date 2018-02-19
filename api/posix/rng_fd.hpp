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
#ifndef INCLUDE_RNG_FD_HPP
#define INCLUDE_RNG_FD_HPP

#include <kernel/rng.hpp>
#include "fd.hpp"

/**
 * @brief      Simple stateless file descriptor
 *             with random device functionality.
 */
class RNG_fd : public FD {
public:
  RNG_fd(int fd)
    : FD{fd} {}

  int read(void* output, size_t bytes) override
  {
    rng_extract(output, bytes);
    return bytes;
  }

  int write(const void* input, size_t bytes) override
  {
    rng_absorb(input, bytes);
    return bytes;
  }

  int close() override
  { return 0; }

}; // < class RNG_fd

#endif
