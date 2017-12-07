#pragma once
#include <stdio.h>

//#define POSIX_STRACE
#ifdef POSIX_STRACE
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif
