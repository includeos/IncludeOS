#include <class_os.hpp>
#include <assert.h>
#include <memstream>

int main();

extern "C" {
  char _BSS_START_, _BSS_END_;
  void _init();
  uint8_t inb(int port);
  void outb(int port, uint8_t data);
  void init_serial();
  int  is_transmit_empty();
  void write_serial(char a);
  void rswrite(char c);  
  void rsprint(const char* ptr);
  
  const int _test_glob = 1;
  
  void enableSSE()
  {
	/*
	 * mov eax, cr0
	 * and ax, 0xFFFB	;clear coprocessor emulation CR0.EM
	 * or ax, 0x2		;set coprocessor monitoring  CR0.MP
	 * mov cr0, eax
	 * mov eax, cr4
	 * or ax, 3 << 9	;set CR4.OSFXSR and CR4.OSXMMEXCPT at the same time
	 * mov cr4, eax 
	*/
    // enable Streaming SIMD Extensions
	__asm__ ("mov %cr0, %eax");
	__asm__ ("and $0xFFFB,%ax");
	__asm__ ("or  $0x2,   %ax");
	__asm__ ("mov %eax, %cr0");
	
	__asm__ ("mov %cr4, %eax");
	__asm__ ("or  $0x600,%ax");
	__asm__ ("mov %eax, %cr4");
  }
  
  void _start(void)
  {    
    __asm__ volatile ("cli");

    // enable SSE extensions bitmask in CR4 register
    enableSSE();
    // init serial port
    init_serial();    
    
    OS::rsprint("\n\n *** IncludeOS Initializing *** \n\n");    
    
    //Initialize .bss secion (It's garbage in qemu)
    OS::rsprint(">>> Initializing .bss... \n");
    streamset8(&_BSS_START_, 0, &_BSS_END_ - &_BSS_START_);
    {
      char* ptr = &_BSS_START_;
      while (ptr < &_BSS_END_)
      {
        if (*ptr)
        {
          rsprint("[ERROR] .bss was not initialized properly with streamset\n");
          //panic(".bss was not initialized");
          asm("cli; hlt;");
        }
        ptr++;
      }
    }
    
    // Call global constructors (relying on .crtbegin to be inserted by gcc)
    _init();
    // verify that global constructors were called
    ASSERT(_test_glob == 1);
    
    OS::rsprint("\n>>> IncludeOS Initialized. Calling main\n");    
    OS::start();
    
    //Will only work if any destructors are called (I think?)
    //    _fini();
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
  
  void rswrite(char c)
  {
    /* Wait for the previous character to be sent */
    while ((inb(0x3FD) & 0x20) != 0x20);
    
    /* Send the character */
    outb(0x3F8, c);
  }
  
  void rsprint(const char* ptr)
  {
    while (*ptr)
      write_serial(*(ptr++));  
  }
}
