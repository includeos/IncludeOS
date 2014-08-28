#include <os>
#include <malloc.h>
#include <stdio.h>
#include <assert.h>
//C++ stuff
void* operator new(size_t size){
  return malloc(size);
}


// A private class to handle IRQ
#include "class_irq_handler.h"


//char huge_array[200]; //{'!'}; Initialize it, puts all the data into the binary.

class global_test
{
  static int calls;
public:
  global_test(){
    calls++;
    if(calls==3)
      OS::rsprint("[PASS]\t Global constructors 1 called 3 times \n");
  }
}globtest1, globtest3,globtest4;

int global_test::calls=0;

//TODO
class global_test2{
 public:
  global_test2(){
    OS::rsprint("[PASS]\t Global constructors 2 called once \n");
  }  
};


class new_obj{
  const char* str="NEW OBJECT: I'm a new, dynamic object!\n";
public:
  new_obj(){
    OS::rsprint(str);
  }
};

extern char _end;
extern int _includeos;
void OS::start(){  
  //assert(7==8);
  char test_end='!';
  rsprint(boot_msg);
  if(&_end<&test_end)
    rsprint("[FAIL]\t _end symbol is less than 'here' \n");
  else
    rsprint("[PASS]\t _end symbol is > 'here'\n");
  rsprint("[PASS]\t OS class 'start' is running - OK\n");
  
  _includeos=100;
  if(&_includeos!=(int*)0xf000 && _includeos==100)
    rsprint("[FAIL]\t _includeos is NOT a cool tool \n");
  else
    rsprint("[PASS]\t _includeos IS a cool tool'\n");

  IRQ_handler::set_IDT();
    
    //Dynamic memory allocation, new
    new_obj* obj=new new_obj();        

    OS::rsprint(">>> Dynamic allocation using malloc \n");
    //Dynamic memory allocation, malloc
    void* mem=0;
    mem=malloc(400);
    OS::rsprint(">>> Malloc done \n");
    //size_t size=0;
    if(mem>NULL)
      rsprint("[PASS]\t Memory allocation using malloc - seems OK\n");
    else
      rsprint("[FAIL]\t Memory allocation using malloc - FAILED\n");
    
    
    int i=55;
    char str[100];
    const char* sprintf_ok="[PASS]\t";
    sprintf(str,"%s sprintf-statement, with an int 55==%i \n",sprintf_ok,i);
    rsprint(str);
    printf("%s Printf is working too... 55==%i\n",sprintf_ok,i);
    Service::start();

    printf("\nMalloced variable mem, is at 0x%x \n",mem);
    printf("\n_end is 0x%x, *_end is 0x%x and _includeos is %x \n",_end,&_end,_includeos);
    halt();    
};

void OS::halt(){
  rsprint("[PASS]\t System halting - OK. Done.\n");
  __asm__ volatile("hlt; jmp _start;");
}

int OS::rsprint(const char* str){
  char* ptr=(char*)str;
  while(*ptr)
    rswrite(*(ptr++));  
  return ptr-str;
}


/* STEAL: Read byte from I/O address space */
uint8_t OS::inb(int port) {  
  int ret;

  __asm__ volatile ("xorl %eax,%eax");
  __asm__ volatile ("inb %%dx,%%al":"=a" (ret):"d"(port));

  return ret;
}


/* STEAL: Write byte to I/O address space */
void OS::outb(int port, uint8_t data) {
  __asm__ volatile ("outb %%al,%%dx"::"a" (data), "d"(port));
}



/* 
 * STEAL: Print to serial port 0x3F8
 */
int OS::rswrite(char c) {
  /* Wait for the previous character to be sent */
  while ((inb(0x3FD) & 0x20) != 0x20);

  /* Send the character */
  outb(0x3F8, c);

  return 1;
}



const char* OS::boot_msg="[PASS]\t Include OS successfully booted (32-bit protected mode)\n"; 


