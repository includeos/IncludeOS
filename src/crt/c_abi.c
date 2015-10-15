#include <stdarg.h>
#include <sys/types.h>
#include <sys/time.h>
#include <utility/memstream.h>
#include <stdio.h>
#include <sys/reent.h>

/// IMPLEMENTATION OF Newlib I/O:
struct _reent newlib_reent;

#undef stdin
#undef stdout
#undef stderr
// instantiate the Unix standard streams
__FILE* stdin;
__FILE* stdout;
__FILE* stderr;

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
  
  /// initialize newlib I/O
  newlib_reent = (struct _reent) _REENT_INIT(newlib_reent);
  // set newlibs internal structure to ours
  _REENT = &newlib_reent;
  // Unix standard streams
  stdin  = _REENT->_stdin;  // stdin  == 1
  stdout = _REENT->_stdout; // stdout == 2
  stderr = _REENT->_stderr; // stderr == 3
  
  /// initialize exceptions before we can run constructors
  extern void* __eh_frame_start;
  // Tell the stack unwinder where exception frames are located
  extern void __register_frame(void*);
  __register_frame(&__eh_frame_start);  
  
  /// call global constructors emitted by compiler
  extern void _init();
  _init();
}

// global/static objects should never be destructed here, so ignore this
void* __dso_handle;

// old function result system
int errno = 0;
int* __errno_location(void)
{
  return &errno;
}

int access(const char *pathname, int mode)
{
	(void) pathname;
  (void) mode;
  
  return 0;
}
char* getcwd(char *buf, size_t size)
{
  (void) buf;
  (void) size;
	return 0;
}
int fcntl(int fd, int cmd, ...)
{
  (void) fd;
  (void) cmd;
	return 0;
}
int fchmod(int fd, mode_t mode)
{
  (void) fd;
  (void) mode;
	return 0;
}
int mkdir(const char *pathname, mode_t mode)
{
  (void) pathname;
  (void) mode;
	return 0;
}
int rmdir(const char *pathname)
{
  (void) pathname;
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
  (void) tv;
  (void) tz;
	return 0;
}
