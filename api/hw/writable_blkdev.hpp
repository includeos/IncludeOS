// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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
#ifndef HW_WRITABLE_BLOCK_DEVICE_HPP
#define HW_WRITABLE__BLOCK_DEVICE_HPP

#include "block_device.hpp"

namespace hw
{
  class Writable_Block_device : public Block_device {
  public:
    /**
     * Write blocks of data to device, IF specially supported
     * This functionality is not enabled by default, nor always supported
    **/
    virtual void write(block_t blk, buffer_t, on_write_func) = 0;
    virtual bool write_sync(block_t blk, buffer_t) = 0;
    
    virtual ~Writable_Block_device() noexcept = default;
  };
}

#endif
