#pragma once
#ifndef VFS_DEVICE_HPP
#define VFS_DEVICE_HPP

#include <cstdint>
#include <sys/types.h>
#include <string>
#include "device.hpp"

namespace hw {
  class VFS_device : public Device {
  public:
    /** Method to get the type of device */
    Device::Type device_type() const noexcept override
    { return Device::Type::Vfs; }

    /** Method to get the name of the device */
    virtual std::string device_name() const override = 0;

    /** Method to get the device's identifier */
    virtual int id() const noexcept = 0;

    /** Method for creating a file handle */
    virtual uint64_t open(char *pathname, uint32_t flags, mode_t mode) = 0;

    /** Method for moving offset to read from without invoking read */
    virtual off_t lseek(uint64_t fh, off_t offset, int whence) = 0;

    /** Method for writing to a file handle */
    virtual ssize_t write(uint64_t fh, void *buf, uint32_t count) = 0;

    /** Method for reading from a file handle */
    virtual ssize_t read(uint64_t fh, void *buf, uint32_t count) = 0;

    /** Method for closing a file handle  */
    virtual int close(uint64_t fh) = 0;
  };
}

#endif