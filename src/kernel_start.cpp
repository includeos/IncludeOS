#include <class_os.hpp>


int main();

extern "C" {
  char _BSS_START_, _BSS_END_;
  void _init();
  uint8_t inb(int port);
  void outb(int port, uint8_t data);
  void init_serial();
  int is_transmit_empty();
  void write_serial(char a);
  void rswrite(char c);  
  void rsprint(const char* ptr);

  char q='?';
  const char* test_msg="TEST MESSAGE";
  
  int test=0x1badbeef;  
  void _start(void){    
    

    __asm__ volatile ("cli");
    __asm__ volatile ("mov $0x1badbeef,%eax");
    __asm__ volatile ("mov $0x1badbeef,%ebx");
    __asm__ volatile ("mov $0x1badbeef,%ecx");
    
    init_serial();    

    OS::rsprint(" \n\n *** IncludeOS Initializing *** \n\n");    
    //Initialize .bss secion (It's garbage in qemu)
    OS::rsprint(">>> Initializing .bss... \n");
    
    
    char* bss=&_BSS_START_;
    *bss=0;
    while(++bss < &_BSS_END_)
      *bss=0;
  
    #ifdef TESTS_H
    test_print_hdr("Global constructors");
    #endif
    //Call global constructors (relying on .crtbegin to be inserted by gcc)
    _init();

    
    OS::rsprint("\n>>> IncludeOS Initialized. Calling main\n");
    
    main();
    
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
  
  #define PORT 0x3f8  
  void init_serial() {
    outb(PORT + 1, 0x00);    // Disable all interrupts
    outb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(PORT + 1, 0x00);    //                  (hi byte)
    outb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
  }
  

  int is_transmit_empty() {
    return inb(PORT + 5) & 0x20;
  }
  
  void write_serial(char a) {
    while (is_transmit_empty() == 0);
    
    outb(PORT,a);
  }

  void rswrite(char c) {
    /* Wait for the previous character to be sent */
    while ((inb(0x3FD) & 0x20) != 0x20);
    
    /* Send the character */
    outb(0x3F8, c);
  }
      
  void rsprint(const char* ptr){
    while(*ptr)
    //for(int i=0;i<10;i++)
      write_serial(*(ptr++));  
  }
  

  int main(){
    OS::start();
    //OBS: If this function returns, the consequences are UNDEFINED
    return 0;
  }
  
}


