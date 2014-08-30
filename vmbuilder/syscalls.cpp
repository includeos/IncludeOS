#include <os>

char *__env[1] = { 0 };
char **environ = __env;

static const int syscall_fd=999;
static bool debug_syscalls=false;


void _exit(){
  OS::rsprint("\tSYSCALL_EXIT\n");
};

int close(int file){
  OS::rsprint("\tSYSCALL_CLOSE\n");
  return -1;
};

#undef errno
int errno=0; //Is this right? 
//Not like in http://wiki.osdev.org/Porting_Newlib

int execve(char *name, char **argv, char **env){
  write(syscall_fd,(char*)"\tSYSCALL EXECVE: WOW- can't do that.\n",25);
  errno=ENOMEM;
  return -1;
};

int fork(){
  write(syscall_fd,(char*)"\tSYSCALL FORK: WOW- can't do that.\n",25);
  kill(1,9);
};

int fstat(int file, struct stat *st){
  write(syscall_fd,(char*)"\tSYSCALL FSTAT: RETURNING DUMMY 0\n",25);
  st->st_mode = S_IFCHR;  
  return 0;
};
int getpid(){
  write(syscall_fd,(char*)"\tSYSCALL GETPID: RETURNING 1\n",25);
  return 1;
};

int isatty(int file){
  write(syscall_fd,(char*)"\tSYSCALL ISATTY: RETURNING 1\n",25);
  return 1;
};
int kill(int pid, int sig){
  write(syscall_fd,(char*)"\tSYSCALL KILL: HALTING\n",25);
  __asm__("cli;hlt;");
};
int link(char *old, char *_new){
  write(syscall_fd,(char*)"\tSYSCALL LINK: CAN'T DO THAT!\n",25);
  kill(1,9);
};
int lseek(int file, int ptr, int dir){
  write(syscall_fd,(char*)"\tSYSCALL LSEEK: RETURNING 0\n",25);
  return 0;
};
int open(const char *name, int flags, ...){
  write(syscall_fd,(char*)"\tSYSCALL OPEN: CAN'T DO THAT - RETURNING -1\n",25);
  return -1;
};
int read(int file, char *ptr, int len){
  write(syscall_fd,(char*)"\tSYSCALL READ: CAN'T DO THAT - RETURNING 0\n",25);
  return 0;
};

int write(int file, char *ptr, int len){
  //char str[100];
  
  if(file==syscall_fd and not debug_syscalls){
    return len;
  }
  /*
    if(debug_syscalls){*/
  //sprintf(str,"\n\t\tWRITE: file: %i debug: %i syscall_fd: %i \n",file,debug_syscalls,syscall_fd);
  //OS::rsprint(str);
    //}
  
  OS::rsprint(ptr);
  return len;
};

extern char _end; // Defined by the linker 
static void* heap_end=(void*)&_end;//(void*)0x1;
caddr_t sbrk(int incr){
  char buf[200]={0};
  write(syscall_fd,(char*)"\tSYSCALL SBRK: Allocating memory\n",25);
  

  //Get the stack pointer
  caddr_t stack_ptr;
  __asm__ ("movl %%esp, %%eax;"
	   "movl %%eax, %0;"
	   :"=r"(stack_ptr)        /* output */
	   :
	   :"%eax"         /* clobbered register */
	   );       
  
  void* prev_heap_end;
  /*sprintf(buf,"\t\tSBRK heap_end==0x%x, incr==0x%x, _end==0x%x, stack_ptr=0x%x \n",(void*)heap_end,incr,&_end,stack_ptr);
    OS::rsprint(buf);*/
  if (heap_end == (void*)0x1) {
    heap_end = &_end;
  }
  
  prev_heap_end = heap_end;

#ifdef TESTS_H
  test_print_result("SBRK increment <= SBRK_MAX",incr <= SBRK_MAX);
#endif



  //Don't exceed the limit
  incr = incr > SBRK_MAX ? SBRK_MAX : incr;
    
  if (heap_end + incr > stack_ptr)
    {
      write (1, (char*)"\t\tERROR: Heap and stack collision\n", 25);
      //abort ();
    }
  
  //Give'm some more!
  /*
  if(incr<0xffff)
  incr=0xffff;*/

  
  heap_end += incr;
  //sprintf(buf,"\t\tSBRK changed heap_end to 0x%x \n",heap_end);
  //OS::rsprint(buf);

  return (caddr_t) prev_heap_end;
}


int stat(const char *file, struct stat *st){
  write(syscall_fd,(char*)"\tSYSCALL STAT: DUMMY\n",25);
  st->st_mode = S_IFCHR;
  return 0;
};

clock_t times(struct tms *buf){
  write(syscall_fd,(char*)"\tSYSCALL TIMES: DUMMY, RETURNING -1\n",25);
  return -1;
};

int unlink(char *name){
  write(syscall_fd,(char*)"\tSYSCALL UNLINK: DUMMY, RETURNING -1\n",25);
  return -1;
};

int wait(int *status){
  write(syscall_fd,(char*)"\tSYSCALL UNLINK: DUMMY, RETURNING -1\n",25);
  return -1;
};


//Compiler says this is allready declared in <sys/time.h>
//int gettimeofday(struct timeval *p, struct timezone *z);
