# LLVM's libc++ for IncludeOS


## Why __linux__ / linux.h
The current version of libc++ (7.0.1) has a toggle for whether or not the `sendfile` system call should be used. Since musl supports this (see `musl/include/sys/sendfile.h` / `musl/src/linux/sendfile.c` in the musl source) IncludeOS should support it too. 

```
#if defined(__linux__)
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
#include <sys/sendfile.h>
#define _LIBCPP_USE_SENDFILE
#endif
```

Snipped from `libcxx/src/filesystem/operations.cpp` in the llvm source.