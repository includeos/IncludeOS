//===------------------------- abort_message.cpp --------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "abort_message.h"
#include "config.h"

#pragma GCC visibility push(hidden)

__attribute__((visibility("hidden"), noreturn))
void abort_message(const char* format, ...)
{
    // record message and call syscall abort_ex
    char* buffer;
    va_list list;
    va_start(list, format);
    vasprintf(&buffer, format, list);
    va_end(list);
    
    abort_ex(buffer);
}

#pragma GCC visibility pop
