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
    // write message to stderr
    // NOTE: should print to 'stderr', not stdout
    va_list list;
    va_start(list, format);
    vfprintf(stdout, format, list);
    va_end(list);
    fprintf(stdout, "\n");
    
    // record message
    char* buffer;
    va_list list2;
    va_start(list2, format);
    vasprintf(&buffer, format, list2);
    va_end(list2);
    
    abort_ex(buffer);
}

#pragma GCC visibility pop
