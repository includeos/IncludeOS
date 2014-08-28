#include <os>

char *__env[1] = { 0 };
char **environ = __env;

static const int syscall_fd=9;
static bool debug_syscalls=true;


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
  write(syscall_fd,(char*)"SYSCALL EXECVE: WOW- can't do that.\n",25);
  errno=ENOMEM;
  return -1;
};

int fork(){
  write(syscall_fd,(char*)"SYSCALL FORK: WOW- can't do that.\n",25);
  kill(1,9);
};

int fstat(int file, struct stat *st){
  write(syscall_fd,(char*)"SYSCALL FSTAT: RETURNING DUMMY 0\n",25);
  st->st_mode = S_IFCHR;  
  return 0;
};
int getpid(){
  write(syscall_fd,(char*)"SYSCALL GETPID: RETURNING 1\n",25);
  return 1;
};

int isatty(int file){
  write(syscall_fd,(char*)"SYSCALL ISATTY: RETURNING 1\n",25);
  return 1;
};
int kill(int pid, int sig){
  write(syscall_fd,(char*)"SYSCALL KILL: HALTING\n",25);
  __asm__("cli;hlt;");
};
int link(char *old, char *_new){
  write(syscall_fd,(char*)"SYSCALL LINK: CAN'T DO THAT!\n",25);
  kill(1,9);
};
int lseek(int file, int ptr, int dir){
  write(syscall_fd,(char*)"SYSCALL LSEEK: RETURNING 0\n",25);
  return 0;
};
int open(const char *name, int flags, ...){
  write(syscall_fd,(char*)"SYSCALL OPEN: CAN'T DO THAT - RETURNING -1\n",25);
  return -1;
};
int read(int file, char *ptr, int len){
  write(syscall_fd,(char*)"SYSCALL READ: CAN'T DO THAT - RETURNING 0\n",25);
  return 0;
};

int write(int file, char *ptr, int len){
  char str[50];
  if(file==syscall_fd and not debug_syscalls){
    return len;
  }
  
  if(debug_syscalls){
    sprintf(str,": file: %i debug: %i syscall_fd: %i \n",file,debug_syscalls,syscall_fd);
    OS::rsprint(str);
  }
   
  OS::rsprint(ptr);
  return len;
};


caddr_t sbrk(int incr){
  
  write(syscall_fd,(char*)"SYSCALL SBRK: Allocating memory\n",25);
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
  char buf[50];
  sprintf(buf,"SBRK changed heap_end to 0x%x \n",prev_heap_end);
  OS::rsprint(buf);
  return (caddr_t) prev_heap_end;
}


int stat(const char *file, struct stat *st){
  write(syscall_fd,(char*)"SYSCALL STAT: DUMMY\n",25);
  st->st_mode = S_IFCHR;
  return 0;
};

clock_t times(struct tms *buf){
  write(syscall_fd,(char*)"SYSCALL TIMES: DUMMY, RETURNING -1\n",25);
  return -1;
};
int unlink(char *name){
  write(syscall_fd,(char*)"SYSCALL UNLINK: DUMMY, RETURNING -1\n",25);
  return -1;
};
int wait(int *status){
  write(syscall_fd,(char*)"SYSCALL UNLINK: DUMMY, RETURNING -1\n",25);
  return -1;
};


//Compiler says this is allready declared in <sys/time.h>
//int gettimeofday(struct timeval *p, struct timezone *z);
