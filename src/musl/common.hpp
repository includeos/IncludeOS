#pragma once

#include <kprint>

#define STUB(X) kprintf("<stubtrace> stubbed syscall %s  called\n", X)

#define STRACE(X, ...) kprintf("<strace> " X, ##__VA_ARGS__)
