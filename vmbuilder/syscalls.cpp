#include "syscalls.h"
#include "class_os.h"

char *__env[1] = { 0 };
char **environ = __env;

void _exit(){
  OS::rsprint("SYSCALL_EXIT\n");
};

int close(int file){
  OS::rsprint("SYSCALL_CLOSE\n");
  return -1;
};

#undef errno
int errno=0; //Is this right? 
//Not like in http://wiki.osdev.org/Porting_Newlib

int execve(char *name, char **argv, char **env){
  errno=ENOMEM;
  return -1;
};

int fork();

int fstat(int file, struct stat *st){
  st->st_mode = S_IFCHR;  
  return 0;
};
int getpid();

int isatty(int file){
  return 1;
};
int kill(int pid, int sig);
int link(char *old, char *_new);
int lseek(int file, int ptr, int dir){
  return 0;
};
int open(const char *name, int flags, ...){
  return -1;
};
int read(int file, char *ptr, int len){
  return 0;
};

int write(int file, char *ptr, int len){
  OS::rsprint(ptr);
  return len;
};


caddr_t sbrk(int incr){
  
  write(1,(char*)"SYSCALL SBRK: Allocating memory\n",25);
  extern char _end; // Defined by the linker 

  //Get the stack pointer
  caddr_t stack_ptr;
  __asm__ ("movl %%esp, %%eax;"
	   "movl %%eax, %0;"
	   :"=r"(stack_ptr)        /* output */
	   :
	   :"%eax"         /* clobbered register */
	   );       
  
  static char *heap_end;
  char *prev_heap_end;
  
  if (heap_end == 0) {
    heap_end = &_end;
  }
  
  prev_heap_end = heap_end;
  if (heap_end + incr > stack_ptr)
    {
      write (1, (char*)"ERROR: Heap and stack collision\n", 25);
      //abort ();
      }
  
  heap_end += incr;
  return (caddr_t) prev_heap_end;
}


int stat(const char *file, struct stat *st){
  st->st_mode = S_IFCHR;
  return 0;
};

clock_t times(struct tms *buf){
  return -1;
};
int unlink(char *name);
int wait(int *status);


//Compiler says this is allready declared in <sys/time.h>
//int gettimeofday(struct timeval *p, struct timezone *z);
