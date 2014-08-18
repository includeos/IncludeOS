#include "class_os.h"


void OS::start(){
    rsprint(boot_msg);
    rsprint("[PASS]\t OS class 'start' running - OK\n");
    Service::start();
    halt();    
};


void OS::halt(){
  rsprint("[PASS]\t System halting - OK.\n");
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
