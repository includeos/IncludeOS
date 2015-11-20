//#define DEBUG
#define MYINFO(X,...) INFO("Kernel",X,##__VA_ARGS__)
#include <os>
#include <stdio.h>
#include <assert.h>

#include <service.hpp>

// A private class to handle IRQ
#include <irq_manager.hpp>
#include <pci_manager.hpp>
#include <stdlib.h>

bool OS::_power = true;
MHz  OS::_CPU_mhz(0);
extern "C" uint16_t _cpu_sampling_freq_divider_;

void OS::start()
{
  debug("  * OS class started\n");
  srand(time(NULL));
  
  // heap
  extern caddr_t heap_end;
  extern char    _end;
  MYINFO("Heap start: %p", heap_end);
  MYINFO("Current end is: %p", &_end);
  asm("cli");  
  //OS::rsprint("  * IRQ handler\n");
  IRQ_manager::init();
  
  // Initialize the Interval Timer
  PIT::init();

  // Initialize PCI devices
  PCI_manager::init();

  asm("sti");
  
  
  MYINFO("Estimating CPU-frequency");
  INFO2("|");
  INFO2("+--(10 samples, %f sec. interval)", 
	(PIT::frequency() / _cpu_sampling_freq_divider_).count());
  INFO2("|");
  
  // TODO: Debug why actual measurments sometimes causes problems. Issue #246. 
  _CPU_mhz = MHz(2200); //PIT::CPUFrequency();

  INFO2("+--> %f MHz", _CPU_mhz.count());
    
  MYINFO("Starting %s",Service::name().c_str());
  FILLINE('=');
  // Everything is ready
  Service::start();
  
  event_loop();
}

void OS::halt(){
  __asm__ volatile("hlt;");
}

double OS::uptime(){  
  return (cycles_since_boot() / Hz(_CPU_mhz).count()) ; 
}

void OS::event_loop()
{

  FILLINE('=');
  printf(" IncludeOS %s \n",version().c_str());
  printf(" +--> Running [ %s ] \n", Service::name().c_str());
  FILLINE('~');


  
  while (_power)
  {
    IRQ_manager::notify(); 
    
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
