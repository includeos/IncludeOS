// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef UTIL_MEMSTREAM_H
#define UTIL_MEMSTREAM_H

#include <sys/types.h>
#include <stdint.h>

#define SSE_SIZE 16
#define SSE_ALIGNED __attribute__ (( aligned (SSE_SIZE) ))
#define SSE_VALIDATE(buffer) (((intptr_t) buffer & (SSE_SIZE-1)) == 0)

/**
 * Allocate SSE-aligned memory
 * Allocates an aligned block of memory to be used with streaming
 * memory functions.
 * 
 * Returns an aligned pointer to allocated memory area.
**/
extern void* aligned_alloc(size_t n, size_t alignment);
#define sse_alloc(n)  aligned_alloc(n, SSE_SIZE)

/**
 * Free SSE-aligned memory
 * 
 * Frees memory allocated by aligned_alloc().
**/
extern void aligned_free(void* ptr);

/**
 * Copy from aligned block of memory
 * Copies the values of n bytes from the SSE-aligned location pointed
 * by source directly to the memory block pointed by destination.
 * 
 * Source and destination cannot overlap by SSE_SIZE bytes.
 * 
 * Returns a pointer to the memory area dest + n.
**/
extern void* streamcpy(void* dest, const void* src, size_t n);

/**
 * Copy from unaligned block of memory
 * Copies the values of n bytes from the unaligned location pointed
 * by source directly to the memory block pointed by destination.
 * 
 * Source and destination cannot overlap by SSE_SIZE bytes.
 * 
 * Returns a pointer to the memory area dest + n.
**/
extern void* streamucpy(void* dest, const void* usrc, size_t n);

/**
 * Fill memory with a constant byte value
 * 
 * The streamset8() function fills the first n bytes of the memory area
 * pointed to by dest with the constant byte value.
 * 
 * Returns a pointer to the memory area dest + n.
**/
extern void* streamset8(void* dest, int8_t value, size_t n);
#define streamset(x, y, z) streamset8(x, y, z)

/**
 * Fill memory with a constant value
 * 
 * The streamset16/32 functions fills n bytes of the memory area
 * pointed to by dest with the constant byte value. The address
 * must be aligned on a 16-byte boundary.
 * 
 * Returns a pointer to the memory area dest + n.
**/
extern void* streamset16(void* dest, int16_t value, size_t n);
extern void* streamset32(void* dest, int32_t value, size_t n);

#endif
