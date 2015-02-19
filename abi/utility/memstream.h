#ifndef UTILITY_MEMSTREAM_H
#define UTILITY_MEMSTREAM_H

#include <sys/types.h>

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
extern void* stream_alloc(size_t n);

/**
 * Free SSE-aligned memory
 * 
 * Frees memory allocated by stream_alloc().
**/
extern void stream_free(void* ptr);

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
 * Fill memory with a constant byte
 * 
 * The streamset() function fills the first n bytes of the memory area
 * pointed to by dest with the constant byte value.
 * 
 * Returns a pointer to the memory area dest + n.
**/
extern void* streamset(void* dest, char value, size_t n);

#endif
