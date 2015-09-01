#include <os>

#include <syscalls.hpp>
#include <string.h>
#include <signal.h>
//#include <vga.hpp>

char *__env[1] = { 0 };
char **environ = __env;

static const int syscall_fd=999;
static bool debug_syscalls=true;
caddr_t heap_end;

//Syscall logger
void syswrite(const char* name, const char* str)
{
  static const char* hdr  = "\tSYSCALL ";
  static const char* term = "\n";
  write(syscall_fd, (char*) hdr,  strlen(hdr));
  write(syscall_fd, (char*) name, strlen(name));
  write(syscall_fd, (char*) ": ", 2);
  write(syscall_fd, (char*) str,  strlen(str));
  write(syscall_fd, (char*) term, strlen(term));
};

void _exit(int status)
{
  printf("\tSYSCALL EXIT: status %d. Nothing more we can do.\n", status);
}

int close(int UNUSED(file)){  
  syswrite("CLOSE","Dummy, returning -1");
  return -1;
};

#undef errno
int errno=0; //Is this right? 
//Not like in http://wiki.osdev.org/Porting_Newlib

int execve(const char* UNUSED(name), 
           char* const* UNUSED(argv), 
           char* const* UNUSED(env))
{
  syswrite((char*) "EXECVE", "NOT SUPPORTED");
  errno = ENOMEM;
  return -1;
};

int fork(){
  syswrite("FORK","NOT SUPPORTED");
  errno=ENOMEM;
  return -1;
};

int fstat(int UNUSED(file), struct stat *st){
  syswrite("FSTAT","Returning OK 0");
  st->st_mode = S_IFCHR;  
  return 0;
};
int getpid(){
  syswrite("GETPID", "RETURNING 1");
  return 1;
};

int isatty(int UNUSED(file))
{
  syswrite("ISATTY","RETURNING 1");
  return 1;
}

int link(const char* UNUSED(old), const char* UNUSED(_new))
{
  syswrite("LINK","CAN'T DO THAT!");
  kill(1,9);
  return -1;
};
int unlink(const char* UNUSED(name)){
  syswrite((char*)"UNLINK","DUMMY, RETURNING -1");
  return -1;
};

off_t lseek(int UNUSED(file), off_t UNUSED(ptr), int UNUSED(dir))
{
  syswrite("LSEEK","RETURNING 0");
  return 0;
}
int open(const char* UNUSED(name), int UNUSED(flags), ...){

  syswrite("OPEN","NOT SUPPORTED - RETURNING -1");
  return -1;
};

int read(int UNUSED(file), void* UNUSED(ptr), size_t UNUSED(len))
{
  syswrite("READ","CAN'T DO THAT - RETURNING 0");
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
  syswrite((char*)"STAT","DUMMY");
  st->st_mode = S_IFCHR;
  return 0;
};

clock_t times(struct tms* UNUSED(buf))
{
  printf("Syscall: times(tms) returning -1\n");
  syswrite((char*)"TIMES","DUMMY, RETURNING -1");
  //__asm__("rdtsc");
  return -1;
};

int wait(int* UNUSED(status)){
  syswrite((char*)"UNLINK","DUMMY, RETURNING -1");
  return -1;
};

int gettimeofday(struct timeval* p, void* UNUSED(z)){
  float seconds = OS::uptime();
  p->tv_sec = int(seconds);
  p->tv_usec = (seconds - p->tv_sec) * 1000000;
  return 5;
}



int kill(pid_t pid, int sig)

{  
  printf("!!! Kill PID: %i, SIG: %i - %s ", pid, sig, strsignal(sig));
  if (sig == (unsigned int)6) printf("/ ABORT \n");
  panic("\t Killing a process doesn't make in IncludeOS. Panic.");
  return -1;
}


// No continuation from here
void panic(const char* why)
{
  printf("\n\t **** PANIC: ****\n %s \n",why);
  printf("\tHeap end: %p \n",heap_end);
  __asm__("cli; hlt;");

}

// to keep our sanity, we need a reason for the abort
void abort_ex(const char* why)
{
  printf("\n\t !!! abort_ex. Why: %s",why);
  panic(why);
}

  /** Added for supporting libc++ */
extern "C" {
  static int __errno__;
  int * __errno_location(void){ return &__errno__; }
}
