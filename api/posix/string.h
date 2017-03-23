#pragma once

#include_next <string.h>

typedef int    errno_t;
typedef size_t rsize_t;

errno_t strerror_s( char *buf, rsize_t bufsz, errno_t errnum );

