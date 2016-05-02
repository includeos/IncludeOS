#pragma once
#ifndef UTILITY_STREAM_HPP
#define UTILITY_STREAM_HPP

#include <net/inet4.hpp>
#include <fs/disk.hpp>
#include <functional>

class Stream
{
  static const size_t PAYLOAD_SIZE = 64000;
  
  typedef net::TCP::Connection_ptr Connection;
  typedef fs::Disk_ptr             Disk;
  typedef fs::FileSystem::Dirent   Dirent;
  
  typedef std::function<void(fs::error_t, bool)> on_after_func;
  
  static void upload(
      Connection, 
      Disk, 
      const Dirent&, 
      on_after_func, 
      size_t = PAYLOAD_SIZE);
};

#endif
