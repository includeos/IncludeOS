#pragma once
#include <stdint.h>
#include <string>

// Index node
typedef int16_t inode_t;

#include <sys/errno.h>

/*
// No such file or directory
#define ENOENT   2
// I/O Error
#define EIO      5
// File already exists
#define EEXIST  17
// Not a directory
#define ENOTDIR 20
// Invalid argument
#define EINVAL  22
// No space left on device
#define ENOSPC  28
// Directory not empty
#define ENOTEMPTY 39
*/

extern std::string fs_error_string(int error);
