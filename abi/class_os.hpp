#ifndef CLASS_OS_H
#define CLASS_OS_H

#include <os>

class OS{
  
 private:  
  
  //The OS will call halt (i.e. wait for interrupts) 
  //once the service is started
  static void halt();  
        
 public:

  //Receive a byte from port
  static uint8_t inb(int port);
  
  //Send a byte to port
  static void outb(int port, uint8_t data);
    
  //Write a cstring to serial port
  static int rsprint(const char* ptr);

  //Write a character to serial port
  static int rswrite(char c);

  //Start the OS
  static void start();


  
};


#endif
