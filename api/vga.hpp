#ifndef OSABI_VGA_HPP
#define OSABI_VGA_HPP

#include <stdint.h>

class ConsoleVGA
{
public:
	typedef unsigned size_t;
	
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
	constexpr static uint8_t make_color(vga_color fg, vga_color bg)
	{
		return fg | bg << 4;
	}
	
	ConsoleVGA();
	
	void write(const char* data, int len);
	void setColor(uint8_t color);
	void clear();
	
	static const size_t VGA_WIDTH  = 80;
	static const size_t VGA_HEIGHT = 25;
	
private:
	void write(char);
	void putEntryAt(char, uint8_t color, size_t x, size_t y);
	void increment(int step);
	void newline();
	
	size_t  row;
	size_t  column;
	uint8_t color;
	uint16_t* buffer;
};
extern ConsoleVGA consoleVGA;

#endif
