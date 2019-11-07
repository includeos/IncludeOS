
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
