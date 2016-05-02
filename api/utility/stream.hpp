#pragma once
#ifndef UTILITY_STREAM_HPP
#define UTILITY_STREAM_HPP

#include <net/inet4.hpp>
#include <fs/disk.hpp>
#include <functional>

class Async
{
  static const size_t PAYLOAD_SIZE = 64000;
  
  typedef net::TCP::Connection_ptr Connection;
  typedef fs::Disk_ptr             Disk;
  typedef fs::FileSystem::Dirent   Dirent;
  
  typedef std::function<void(bool)> next_func;
  typedef std::function<void(fs::error_t, bool)> on_after_func;
  typedef std::function<void(fs::buffer_t, size_t, next_func)> on_write_func;
  
  static void upload_file(
      Disk, 
      const Dirent&, 
      Connection, 
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
