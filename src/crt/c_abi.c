#include <stdarg.h>
#include <sys/types.h>
#include <sys/time.h>

int access(const char *pathname, int mode)
{
	return 0;
}
char* getcwd(char *buf, size_t size)
{
	return 0;
}
int fcntl(int fd, int cmd, _VA_LIST_...)
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


// We expect the linker script to tell us where eception headers are
void*  __eh_frame_start;

// This is provided by ... I think libgcc
void __register_frame ( void * );

void _init_c_runtime(){
  
  // Tell the stack unwinder where exception frames are located
  __register_frame( &__eh_frame_start );  
  
    // Call global constructors (relying on .crtbegin to be inserted by gcc)
  _init();
  


}
