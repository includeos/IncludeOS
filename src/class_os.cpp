#include <os>
#include <malloc.h>
#include <stdio.h>
#include <assert.h>

#include <class_dev.hpp>
#include <class_service.hpp>

// A private class to handle IRQ
#include "class_irq_handler.hpp"
#include <class_pci_manager.hpp>

//char huge_array[200]; //{'!'}; 
//Initialize it, puts all the data into the binary.
#ifdef TESTS_H
class global_test
{
public:
  static int calls;
  
  global_test()
  {
	calls++;
	test_print_result("Global constructor 1 is called", calls);
	if (calls >= 3)
	{
	  test_print_result(
		"Global constructor 1 should be called 3 times", calls == 3);
	}
  }
  
} globtest1, globtest3, globtest4;

int global_test::calls = 0;

//TODO
class global_test2{
 public:
  global_test2(){
    test_print_result("Global constructor 2 is called once",true);
  }  
}globtest2;

#endif //TESTS_H

bool OS::power = true;

extern char _end;
extern int _includeos;

int glob1=0;
int glob2=8888;

void OS::start()
{
  rsprint(">>> OS class started\n");
#ifdef TESTS_H
  test_print_result("Static variables have been properly initialized",
		    stat1==0 && stat2==7777);
  test_print_result("Global variables have been properly initialized",
		    glob1==0 && glob2==8888);  
  test_malloc();
  test_new();
  test_string();
  test_sprintf();
  test_printf();
#endif
  
  __asm__("cli");  
  IRQ_handler::init();
  Dev::init();  
  
  //Everything is ready
  Service::start();
  
  __asm__("sti");
  halt();
};

extern "C" void halt_loop(){
  __asm__ volatile("hlt; jmp halt_loop;");
}

void OS::halt(){
  OS::rsprint("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
  OS::rsprint(">>> System idle - everything seems OK \n");
  while(power){        
    //DEBUG Disable interrupts while notifying
    //__asm__ volatile("cli");
    IRQ_handler::notify(); 
    //__asm__ volatile("sti");
    __asm__ volatile("hlt;");
    printf("<OS> Woke up! \n");
  }
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


/*  Write byte to I/O address space */
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






