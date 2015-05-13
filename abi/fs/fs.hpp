#pragma once

#include <stdint.h>
#include <string>
#include "common.hpp"

namespace fs
{
  #pragma interface   // __declspec(novtable)
  class FilesystemBase
  {
  public:
    virtual inode_t
    open(const std::string& name, int flags, ...) = 0;
    virtual int
    read(inode_t file, const std::string& ptr, int len) = 0;
    virtual int
    write(inode_t file, const std::string& ptr, int len) = 0;
    virtual int
    close(inode_t file) = 0;
    
    virtual inode_t
    symlink(const std::string& from, const std::string& to) = 0;
    virtual int
    stat(const std::string& path, void* buf) = 0;
    
    virtual int
    readdir(const std::string& path, void* ptr) = 0;
    virtual int
    rmdir(const std::string& path) = 0;
    virtual int
    mkdir(const std::string& path) = 0;
    
  };
  
}
