#include "cxx_abi.hpp"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

void* operator new (size_t size)
{
    return malloc(size);
}
void* operator new[] (size_t size)
{
    return malloc(size);
}

void operator delete (void* p)
{
    free(p);
}
void operator delete[] (void* p)
{
    free(p);
}

// placement new/delete
void* operator new  (size_t, void* p) throw() { return p; }
void* operator new[](size_t, void* p) throw() { return p; }
void  operator delete  (void*, void*) throw() { }
void  operator delete[](void*, void*) throw() { }

// EASTL new/delete
//void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
void* operator new[](size_t size, const char*, int, unsigned, const char*, int)
{
	//printf("[new:%lu] %s %d %d from %s:%d\n", size, pName, flags, debugFlags, file, line);
	return malloc(size);
}
//void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
void* operator new[](size_t size, size_t, size_t, const char*, int, unsigned, const char*, int)
{
	//printf("[new:%lu] %s %d %d from %s:%d\n", size, pName, flags, debugFlags, file, line);
	return malloc(size);
}


extern "C"
{
	void __cxa_pure_virtual()
	{
		// Do nothing or print an error message.
		printf("@cxa_pure_virtual: Pure virtual function missing\n");
	}
	
	// NOTE: newlib implements __cxa_atexit & __cxa_finalize
	
	
} // extern
