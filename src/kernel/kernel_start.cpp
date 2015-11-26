#include <kernel/os.hpp>
#include <assert.h>
#include <debug>
//#define DEBUG

extern "C"
{
  void _init_c_runtime();
  uint8_t inb(int port);
  void outb(int port, uint8_t data);
  void init_serial();
  int  is_transmit_empty();
  void write_serial(char a);
  
#ifdef DEBUG
  const int _test_glob = 123;
  int _test_constructor = 0;
#endif
  
  // enables Streaming SIMD Extensions
  static void enableSSE(void)
  {
    __asm__ ("mov %cr0, %eax");
    __asm__ ("and $0xFFFB,%ax");
    __asm__ ("or  $0x2,   %ax");
    __asm__ ("mov %eax, %cr0");
    
    __asm__ ("mov %cr4, %eax");
    __asm__ ("or  $0x600,%ax");
    __asm__ ("mov %eax, %cr4");
  }
  
#ifdef DEBUG
  __attribute__((constructor)) void test_constr()
  {    
    OS::rsprint("\t * C constructor was called!\n");
    _test_constructor = 1;
  }
#endif
  
  void _start(void)
  {    
    __asm__ volatile ("cli");
    
    // enable SSE extensions bitmask in CR4 register
    enableSSE();
    
    // init serial port
    init_serial();    
    
    // @note: Printing before the c-runtime is initialized can only be done like so:
    // OS::rsprint("Booting...");
    
    // Initialize stack-unwinder, call global constructors etc.
    #ifdef DEBUG
      OS::rsprint("\t * Initializing C-environment... \n");
    #endif
    _init_c_runtime();
    // we can't initialize it before we are here
    OS::set_rsprint_secondary([] (const char*, size_t) {});
    
    FILLINE('=');
    CAPTION("#include<os> // Literally");
    FILLINE('=');
 
    
// verify that global constructors were called
#ifdef DEBUG
    assert(_test_glob == 123);
    assert(_test_constructor == 1);
#endif
    // Initialize some OS functionality
    OS::start();
    
    // Will only work if any destructors are called (I think?)
    //_fini();
  }
  
  uint8_t inb(int port) {  
    int ret;
    
    __asm__ volatile ("xorl %eax,%eax");
    __asm__ volatile ("inb %%dx,%%al":"=a" (ret):"d"(port));
    
    return ret;
  }
  
  void outb(int port, uint8_t data) {
    __asm__ volatile ("outb %%al,%%dx"::"a" (data), "d"(port));
  }
  
  #define SERIAL_PORT 0x3f8  
  void init_serial() {
    outb(SERIAL_PORT + 1, 0x00);    // Disable all interrupts
    outb(SERIAL_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(SERIAL_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(SERIAL_PORT + 1, 0x00);    //                  (hi byte)
    outb(SERIAL_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(SERIAL_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(SERIAL_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
  }
  
  int is_transmit_empty() {
    return inb(SERIAL_PORT + 5) & 0x20;
  }
  
  void write_serial(char a) {
    while (is_transmit_empty() == 0);
    
    outb(SERIAL_PORT, a);
  }
}
