#ifndef CLASS_OS_H
#define CLASS_OS_H

#include <common>


/** The entrypoint for OS services
    
    @note For device access, see Dev
 */
class OS{
  
 private:  

  static bool power;
  
  /** The OS will call halt (i.e. wait for interrupts) once the 
      service is started */
  static void halt();  
  
  static uint32_t _CPU_mhz;

 public: 

  /** Clock cycles since boot. */
  static inline uint64_t rdtsc()
  {
    uint64_t ret;
    __asm__ volatile ("rdtsc":"=A"(ret));
    return ret;
  }

  /** Uptime in seconds. */
  static inline uint32_t uptime()
  { return (rdtsc() / _CPU_mhz) / 1000000; }
  
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
