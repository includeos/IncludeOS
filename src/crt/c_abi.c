#include <stdarg.h>
#include <sys/types.h>
#include <sys/time.h>
#include <utility/memstream.h>
#include <stdio.h>

void _init_c_runtime()
{
  // Initialize .bss section
  extern char _BSS_START_, _BSS_END_;
  streamset8(&_BSS_START_, 0, &_BSS_END_ - &_BSS_START_);
  
  // Initialize the heap before exceptions
  extern caddr_t heap_end; // used by SBRK:
  extern char _end;        // Defined by the linker 
  // Set heap to after _end (given by linker script) if needed
  if (&_end > heap_end)
    heap_end = &_end;
  
  // Initialize exceptions before we can run constructors
  extern void* __eh_frame_start;
  // Tell the stack unwinder where exception frames are located
  extern void __register_frame(void*);
  __register_frame(&__eh_frame_start);  
  
  // Call global constructors (relying on .crtbegin to be inserted by gcc)
  extern void _init();
  _init();
}

// global/static objects should never be destructed here, so ignore this
void* __dso_handle;
// run global constructors
typedef void (*constr_func)();
void _init()
{
  extern constr_func _GCONSTR_START;
  extern constr_func _GCONSTR_END;
  
  printf("Calling global constructors from %p to %p\n", 
      &_GCONSTR_START, &_GCONSTR_END);
  
  for(constr_func* gc = &_GCONSTR_START; gc != &_GCONSTR_END; gc++)
  {
    (*gc)();
  }
}

// old function result system
int errno = 0;
int* __errno_location(void)
{
  return &errno;
}

int access(const char *pathname, int mode)
{
	return 0;
}
char* getcwd(char *buf, size_t size)
{
	return 0;
}
int fcntl(int fd, int cmd, ...)
{
	return 0;
}
int fchmod(int fildes, mode_t mode)
{
	return 0;
}
int mkdir(const char *pathname, mode_t mode)
{
	return 0;
}
int rmdir(const char *pathname)
{
	return 0;
}

/*
int gettimeofday(struct timeval *__restrict tv,
				 void *__restrict tz)
{
	return 0;
}*/

int settimeofday(const struct timeval *tv, const struct timezone *tz)
{
	return 0;
}
