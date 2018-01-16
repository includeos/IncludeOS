#pragma once

#include <kprint>

#define STUB(X) kprintf("syscall %s called\n", X)

#define STRACE(X, ...) kprintf(X, ##__VA_ARGS__)
