#include <os>
#include <class_dev.hpp>

enum vga_color
  {
    COLOR_BLACK = 0,
    COLOR_BLUE = 1,
    COLOR_GREEN = 2,
    COLOR_CYAN = 3,
    COLOR_RED = 4,
    COLOR_MAGENTA = 5,
    COLOR_BROWN = 6,
    COLOR_LIGHT_GREY = 7,
    COLOR_DARK_GREY = 8,
    COLOR_LIGHT_BLUE = 9,
    COLOR_LIGHT_GREEN = 10,
    COLOR_LIGHT_CYAN = 11,
    COLOR_LIGHT_RED = 12,
    COLOR_LIGHT_MAGENTA = 13,
    COLOR_LIGHT_BROWN = 14,
    COLOR_WHITE = 15,
  };
 
uint8_t make_color(enum vga_color fg, enum vga_color bg)
{
  return fg | bg << 4;
}
 
uint16_t make_vgaentry(char c, uint8_t color)
{
  uint16_t c16 = c;
  uint16_t color16 = color;
  return c16 | color16 << 8;
}
 
size_t strlen(const char* str)
{
  size_t ret = 0;
  while ( str[ret] != 0 )
    ret++;
  return ret;
}
 
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
 
size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;
 
void terminal_initialize()
{
  terminal_row = 0;
  terminal_column = 0;
  terminal_color = make_color(COLOR_WHITE, COLOR_RED);
  terminal_buffer = (uint16_t*) 0xB8000;
  for ( size_t y = 0; y < VGA_HEIGHT; y++ )
    {
      for ( size_t x = 0; x < VGA_WIDTH; x++ )
        {
          const size_t index = y * VGA_WIDTH + x;
          terminal_buffer[index] = make_vgaentry(' ', terminal_color);
        }
    }
}
 
void terminal_setcolor(uint8_t color)
{
  terminal_color = color;
}
 
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y)
{
  const size_t index = y * VGA_WIDTH + x;
  terminal_buffer[index] = make_vgaentry(c, color);
}
 
void terminal_putchar(char c)
{
  terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
  if ( ++terminal_column == VGA_WIDTH )
    {
      terminal_column = 0;
      if ( ++terminal_row == VGA_HEIGHT )
        {
          terminal_row = 0;
        }
    }
}
 
void terminal_writestring(const char* data)
{
  size_t datalen = strlen(data);
  for ( size_t i = 0; i < datalen; i++ )
    terminal_putchar(data[i]);
}


void Service::start(){
  printf("\n *** Service is up - with OS Included! *** \n");
  
  auto eth1=Dev::eth(0);
  //auto eth2=Dev::eth<VirtioNet>(1);
  
  printf("Using ethernet device: %s \n", eth1.name());

  printf("Mac: %s \n",eth1.mac_str());
  
  printf("Testing video memory \n");
  
  
  terminal_initialize();
  terminal_row = 12;
  terminal_column = 25;
  terminal_writestring("#include <os>");
  terminal_setcolor(make_color(COLOR_LIGHT_GREY,COLOR_RED));
  terminal_writestring(" // Yea, really!");

  uint8_t color=0;
  for (size_t row = 14; row < 18; row++)
    for (size_t col = 0; col < VGA_WIDTH; col++)
      terminal_putentryat(' ',color+=8,col,row);
  
  printf("Done!\n");
}

