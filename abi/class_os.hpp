#ifndef CLASS_OS_H
#define CLASS_OS_H

#include <common>
#include <assert.h>
#include <class_pit.hpp>

/** The entrypoint for OS services
    
    @note For device access, see Dev
 */
class OS{
  
 public:     
  
  // No copy or move
  OS(OS&) = delete;
  OS(OS&&) = delete;
  
  // No construction
  OS() = delete;

  
  /** Clock cycles since boot. */
  static inline uint64_t cycles_since_boot()
  {
    uint64_t ret;
    __asm__ volatile ("rdtsc":"=A"(ret));
    return ret;
  }
  
  /** Uptime in seconds. */
  static inline float uptime()
  { return (cycles_since_boot() / _CPU_mhz) / 1000000; }
  
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

  
  /** Halt until next inerrupt. 
      @Warning If there is no regular timer interrupt (i.e. from PIT / APIC) 
      we'll stay asleep. 
   */
  static void halt();  


  // PRIVATE
private:  
  
  /** Indicate if the OS is running. */
  static bool _power;
  
  /** The main event loop.  Check interrupts, timers etc., and do callbacks. */
  static void event_loop();
  
  static double _CPU_mhz;

  
  
};


#endif
