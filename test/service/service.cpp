typedef unsigned char uint8_t;

uint8_t inb(int port);
void outb(int port, uint8_t data);
int rswrite(const void *drop, char c);


extern "C" void _start(void){
  void* v=0;
  rswrite(v,'!');
  __asm__ volatile("hlt; jmp _start;");
}


/* STEAL: Read byte from I/O address space */
uint8_t    inb(int port) {  
  int ret;

  __asm__ volatile ("xorl %eax,%eax");
  __asm__ volatile ("inb %%dx,%%al":"=a" (ret):"d"(port));

  return ret;
}


/* STEAL: Write byte to I/O address space */
void    outb(int port, uint8_t data) {
  __asm__ volatile ("outb %%al,%%dx"::"a" (data), "d"(port));
}


/* 
 * STEAL: Print to serial port 0x3F8
 */
int rswrite(const void *drop, char c) {
  /* Wait for the previous character to be sent */
  while ((inb(0x3FD) & 0x20) != 0x20);

  /* Send the character */
  outb(0x3F8, c);

  return 1;
}
