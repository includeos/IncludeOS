#pragma once
#ifndef DAX_DEVICE_HPP
#define DAX_DEVICE_HPP

#include <cstdint>
#include <string>
#include "device.hpp"

namespace hw {
  class DAX_device : public Device {
  public:
    /** Method to get the type of device */
    Device::Type device_type() const noexcept override
    { return Device::Type::Dax; }

    /** Method to get the name of the device */
    virtual std::string device_name() const override = 0;

    /** Method to get the device's identifier */
    virtual int id() const noexcept = 0;

    /** Method for grabbing PMEM start */
    virtual void *start_addr() const noexcept = 0;

    /** Method for grabbing PMEM size */
    virtual uint64_t size() const noexcept = 0;
  };
}

#endif