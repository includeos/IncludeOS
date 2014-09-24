#ifndef CLASS_OS_H
#define CLASS_OS_H

#include <common>


/** The entrypoint for OS services
    
    @note For device access, see Dev
 */
class OS{
  
 private:  
  
  /** The OS will call halt (i.e. wait for interrupts) once the 
      service is started */
  static void halt();  
        
 public:

  /** Receive a byte from port. @todo Should be moved */
  static uint8_t inb(int port);
  
  /** Send a byte to port. @todo Should be moved to hw/...something */
  static void outb(int port, uint8_t data);
    
  /** Write a cstring to serial port. @todo Should be moved to Dev::serial(n).*/
  static int rsprint(const char* ptr);

  /** Write a character to serial port. @todo Should be moved Dev::serial(n) */
  static int rswrite(char c);

  /** Start the OS.  @todo Should be `init()` - and not accessible from ABI */
  static void start();


  
};


#endif
