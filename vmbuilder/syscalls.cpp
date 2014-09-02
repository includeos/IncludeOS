#include <os>
#include <string.h>

char *__env[1] = { 0 };
char **environ = __env;

static const int syscall_fd=999;
static bool debug_syscalls=true;


//Syscall logger
void syswrite(char* name,char* str){ 
  static char* hdr="\tSYSCALL ";
  static char* term="\n";
  write(syscall_fd,hdr,strlen(hdr));
  write(syscall_fd,(char*)name,strlen(name));
  write(syscall_fd,": ",2);
  write(syscall_fd,(char*)str,strlen(str));
  write(syscall_fd,term,strlen(term));
};




void _exit(){
  syswrite("EXIT","Not implemented");
};

int close(int file){
  syswrite("CLOSE","Dummy, returning -1");
  return -1;
};

#undef errno
int errno=0; //Is this right? 
//Not like in http://wiki.osdev.org/Porting_Newlib

int execve(char *name, char **argv, char **env){
  syswrite((char*)"EXECVE","NOT SUPPORTED");
  errno=ENOMEM;
  return -1;
};

int fork(){
  syswrite("FORK","NOT SUPPORTED");
  errno=ENOMEM;
  return -1;
};

int fstat(int file, struct stat *st){
  syswrite("FSTAT","Returning OK 0");
  st->st_mode = S_IFCHR;  
  return 0;
};
int getpid(){
  syswrite("GETPID", "RETURNING 1");
  return 1;
};

int isatty(int file){
  syswrite("ISATTY","RETURNING 1");
  return 1;
};
int kill(int pid, int sig){
  syswrite("KILL","HALTING");
  __asm__("cli;hlt;");
};
int link(char *old, char *_new){
  syswrite("LINK","CAN'T DO THAT!");
  kill(1,9);
};
int lseek(int file, int ptr, int dir){
  syswrite("LSEEK","RETURNING 0");
  return 0;
};
int open(const char *name, int flags, ...){
  syswrite("OPEN","NOT SUPPORTED - RETURNING -1");
  return -1;
};
int read(int file, char *ptr, int len){
  syswrite("READ","CAN'T DO THAT - RETURNING 0");
  return 0;
};

int write(int file, char *ptr, int len){
  //char str[100];
  
  if(file==syscall_fd and not debug_syscalls){
    return len;
  }
  
  for(int i=0;i<len;i++)
    OS::rswrite(*ptr++);

  /*
    if(debug_syscalls){*/
  //sprintf(str,"\n\t\tWRITE: file: %i debug: %i syscall_fd: %i \n",file,debug_syscalls,syscall_fd);
  //OS::rsprint(str);
    //}
    
  //OS::rsprint(ptr);
  return len;
};

extern char _end; // Defined by the linker 
static void* heap_end=(void*)&_end;//(void*)0x1;
caddr_t sbrk(int incr){
  char buf[200]={0};
  syswrite("SBRK","Allocating memory");
  

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
  if (heap_end == (void*)0x0) {
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
  syswrite((char*)"STAT","DUMMY");
  st->st_mode = S_IFCHR;
  return 0;
};

clock_t times(struct tms *buf){
  syswrite((char*)"TIMES","DUMMY, RETURNING -1");
  //__asm__("rdtsc");
  return -1;
};

int unlink(char *name){
  syswrite((char*)"UNLINK","DUMMY, RETURNING -1");
  return -1;
};

int wait(int *status){
  syswrite((char*)"UNLINK","DUMMY, RETURNING -1");
  return -1;
};


//Compiler says this is allready declared in <sys/time.h>
//int gettimeofday(struct timeval *p, struct timezone *z);
