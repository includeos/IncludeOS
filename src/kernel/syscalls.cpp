#include <os>

#include <syscalls.hpp>
#include <string.h>
#include <signal.h>

char *__env[1] = { 0 };
char **environ = __env;

static const int syscall_fd=999;
static bool debug_syscalls=true;
caddr_t heap_end;

void _exit(int status)
{
  printf("\tSYSCALL EXIT: status %d. Nothing more we can do.\n", status);
  printf("\tSTOPPING EXECUTION\n");
  while (1)
    asm("cli; hlt;");
}

int close(int UNUSED(file)){  
  debug("SYSCALL CLOSE Dummy, returning -1");
  return -1;
};

int execve(const char* UNUSED(name), 
           char* const* UNUSED(argv), 
           char* const* UNUSED(env))
{
  debug((char*) "SYSCALL EXECVE NOT SUPPORTED");
  errno = ENOMEM;
  return -1;
};

int fork(){
  debug("SYSCALL FORK NOT SUPPORTED");
  errno=ENOMEM;
  return -1;
};

int fstat(int UNUSED(file), struct stat *st){
  debug("SYSCALL FSTAT Dummy, returning OK 0");
  st->st_mode = S_IFCHR;  
  return 0;
};
int getpid(){
  debug("SYSCALL GETPID Dummy, returning 1");
  return 1;
};

int isatty(int file)
{
  if (file == 1 || file == 2 || file == 3)
  {
    debug("SYSCALL ISATTY Dummy returning 1");
    return 1;
  }
  // not stdxxx, error out
  debug("SYSCALL ISATTY Unknown descriptor %i", file);
  errno = EBADF;
  return 0;
}

int link(const char* UNUSED(old), const char* UNUSED(_new))
{
  debug("SYSCALL LINK - Unsupported");
  kill(1,9);
  return -1;
}
int unlink(const char* UNUSED(name))
{
  debug((char*)"SYSCALL UNLINK Dummy, returning -1");
  return -1;
}

off_t lseek(int UNUSED(file), off_t UNUSED(ptr), int UNUSED(dir))
{
  debug("SYSCALL LSEEK returning 0");
  return 0;
}
int open(const char* UNUSED(name), int UNUSED(flags), ...){

  debug("SYSCALL OPEN Dummy, returning -1");
  return -1;
};

int read(int UNUSED(file), void* UNUSED(ptr), size_t UNUSED(len))
{
  debug("SYSCALL READ Not supported, returning 0");
  return 0;
}
int write(int file, const void* ptr, size_t len)
{
  if (file == syscall_fd and not debug_syscalls)
		return len;
	
	// VGA console output
	//consoleVGA.write(ptr, len);
	
	// serial output
	for(size_t i = 0; i < len; i++)
		OS::rswrite( ((char*) ptr)[i] );
	
	return len;
}

void* sbrk(ptrdiff_t incr)
{
  void* prev_heap_end = heap_end;
  heap_end += incr;
  return (caddr_t) prev_heap_end;
}


int stat(const char* UNUSED(file), struct stat *st){
  debug("SYSCALL STAT Dummy");
  st->st_mode = S_IFCHR;
  return 0;
};

clock_t times(struct tms* UNUSED(buf))
{
  debug((char*)"SYSCALL TIMES Dummy, returning -1");

  return -1;
};

int wait(int* UNUSED(status)){
  debug((char*)"SYSCALL WAIT Dummy, returning -1");
  return -1;
};

int gettimeofday(struct timeval* p, void* UNUSED(z)){
  // Currently every reboot takes us back to 1970 :-)
  float seconds = OS::uptime();
  p->tv_sec = int(seconds);
  p->tv_usec = (seconds - p->tv_sec) * 1000000;
  return 5;
}



int kill(pid_t pid, int sig)
{  
  printf("!!! Kill PID: %i, SIG: %i - %s ", pid, sig, strsignal(sig));
  
  if (sig == 6ul)
    printf("/ ABORT \n");
  
  panic("\tKilling a process doesn't make sense in IncludeOS. Panic.");
  errno = ESRCH;
  return -1;
}

// No continuation from here
void panic(const char* why)
{
  printf("\n\t **** PANIC: ****\n %s \n", why);
  printf("\tHeap end: %p \n", heap_end);
  while(1) __asm__("cli; hlt;");
  
}

// to keep our sanity, we need a reason for the abort
void abort_ex(const char* why)
{
  printf("\n\t !!! abort_ex. Why: %s", why);
  panic(why);
}
