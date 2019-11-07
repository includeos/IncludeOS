
#pragma once
#ifndef UTIL_ASYNC_HPP
#define UTIL_ASYNC_HPP

#include <net/inet>
#include <net/stream.hpp>
#include <fs/disk.hpp>

class Async
{
public:
  static const size_t PAYLOAD_SIZE = 64000;

  using Stream  = net::Stream;
  using Disk    = fs::Disk_ptr;
  using Dirent  = fs::Dirent;

  typedef delegate<void(bool)> next_func;
  typedef delegate<void(fs::error_t, bool)> on_after_func;
  typedef delegate<void(fs::buffer_t, next_func)> on_write_func;

  static void upload_file(
      Disk,
      const Dirent&,
      Stream*,
      on_after_func,
      size_t = PAYLOAD_SIZE);

  static void disk_transfer(
      Disk,
      const Dirent&,
      on_write_func,
      on_after_func,
      size_t = PAYLOAD_SIZE);

};

#endif
