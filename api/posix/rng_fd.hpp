
#pragma once
#ifndef INCLUDE_RNG_FD_HPP
#define INCLUDE_RNG_FD_HPP

#include <kernel/rng.hpp>
#include "fd.hpp"
#include <sys/stat.h>

/**
 * @brief      Simple stateless file descriptor
 *             with random device functionality.
 */
class RNG_fd : public FD {
public:
  RNG_fd(int fd)
    : FD{fd} {}

  ssize_t read(void* output, size_t bytes) override
  {
    rng_extract(output, bytes);
    return bytes;
  }

  int write(const void* input, size_t bytes) override
  {
    rng_absorb(input, bytes);
    return bytes;
  }

  long fstat(struct stat* buf) override
  {
    buf->st_dev = 0x6;
    buf->st_ino = 10;
    buf->st_nlink = 1;
    buf->st_size = 0;
    buf->st_atime = 0;
    buf->st_ctime = 0;
    buf->st_mtime = 0;
    buf->st_blocks = 0;
    buf->st_blksize = 4096;
    return 0;
  }

  int close() override
  { return 0; }

}; // < class RNG_fd

#endif
