//===----------------------------- config.h -------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//
//  Defines macros used within the libc++abi project.
//
//===----------------------------------------------------------------------===//


#ifndef LIBCXXABI_CONFIG_H
#define LIBCXXABI_CONFIG_H

#include <unistd.h>

// much better than abort()
void abort_ex(const char*) __attribute__((noreturn));

// Set this in the CXXFLAGS when you need it
#define LIBCXXABI_HAS_NO_THREADS 1

// Set this in the CXXFLAGS when you need it, because otherwise we'd have to
// #if !defined(__linux__) && !defined(__APPLE__) && ...
// and so-on for *every* platform.
#define LIBCXXABI_BAREMETAL 1

#endif // LIBCXXABI_CONFIG_H
