//#define DEBUG
#include <os>
#include <stdio.h>
#include <assert.h>

#include <class_service.hpp>

// A private class to handle IRQ
#include "class_irq_handler.hpp"
#include <class_pci_manager.hpp>
#include <stdlib.h>

bool  OS::_power = true;
float OS::_CPU_mhz = 2399.928; //For Trident3, reported by /proc/cpuinfo
// used by SBRK:
extern caddr_t heap_end;

extern "C"
{
  #undef stdin
  #undef stdout
  #undef stderr
  extern __FILE* stdin;
  extern __FILE* stdout;
  extern __FILE* stderr;
  
  extern struct _reent stuff;
}

void OS::start()
{
  // Set heap to an appropriate location
  if (&_end > heap_end)
    heap_end = &_end;
  
  rsprint(">>> OS class started\n");
  srand(time(NULL));
  
  // Disable the timer interrupt completely
  disable_PIT();
  
  // initialize C-library?
  // this part im not sure about
  _REENT = &stuff;
  // this part is correct
  stdin  = _REENT->_stdin;
  stdout = _REENT->_stdout;
  stderr = _REENT->_stderr;
  
  // heap
  printf("<OS> Heap start: %p\n", heap_end);
  
  timeval t;
  gettimeofday(&t,0);
  printf("<OS> TimeOfDay: %li.%li Uptime: %f \n",
      t.tv_sec, t.tv_usec, uptime());
  
  asm("cli");  
  //OS::rsprint(">>> IRQ handler\n");
  IRQ_handler::init();
  //OS::rsprint(">>> Dev init\n");
  Dev::init();
  
  // Everything is ready
  printf(">>> IncludeOS initialized - calling Service::start()\n");
  Service::start();
  
  asm("sti");
  halt();
}

void OS::disable_PIT()
{
  #define PIT_one_shot 0x30
  #define PIT_mode_chan 0x43
  #define PIT_chan0 0x40
  
  // Enable 1-shot mode
  OS::outb(PIT_mode_chan, PIT_one_shot);
  
  // Set a frequency for "first shot"
  OS::outb(PIT_chan0, 1);
  OS::outb(PIT_chan0, 0);
  debug("<PIT> Switching to 1-shot mode (0x%x) \n",PIT_one_shot);
}

extern "C" void halt_loop(){
  __asm__ volatile("hlt; jmp halt_loop;");
}

void OS::halt()
{
  OS::rsprint("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
  OS::rsprint(">>> System idle - waiting for interrupts \n");
  OS::rsprint("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
  
  while (_power)
  {
    IRQ_handler::notify(); 
    
    debug("<OS> Woke up @ t = %li \n",uptime());
  }
  
  //Cleanup
  //Service::stop();
}

int OS::rsprint(const char* str)
{
  int len = 0;
  while (str[len])
    rswrite(str[len++]);
  
  return len;
}


/* STEAL: Read byte from I/O address space */
uint8_t OS::inb(int port)
{
  int ret;
  __asm__ volatile ("xorl %eax,%eax");
  __asm__ volatile ("inb %%dx,%%al":"=a" (ret):"d"(port));

  return ret;
}

/*  Write byte to I/O address space */
void OS::outb(int port, uint8_t data) {
  __asm__ volatile ("outb %%al,%%dx"::"a" (data), "d"(port));
}

/* 
 * STEAL: Print to serial port 0x3F8
 */
int OS::rswrite(char c)
{
  /* Wait for the previous character to be sent */
  while ((inb(0x3FD) & 0x20) != 0x20);

  /* Send the character */
  outb(0x3F8, c);

  return 1;
}
