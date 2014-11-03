#include <vga.hpp>

#include <string.h>

ConsoleVGA consoleVGA;

uint16_t make_vgaentry(char c, uint8_t color)
{
	uint16_t c16 = c;
	uint16_t color16 = color;
	return c16 | color16 << 8;
}

ConsoleVGA::ConsoleVGA()
{
	this->row = 0;
	this->column = 0;
	this->color = make_color(COLOR_WHITE, COLOR_BLACK);
	this->buffer = (uint16_t*) 0xB8000;
	
	clear();
}

void ConsoleVGA::setColor(uint8_t color)
{
	this->color = color;
}

void ConsoleVGA::putEntryAt(char c, uint8_t color, size_t x, size_t y)
{
	const size_t index = y * VGA_WIDTH + x;
	this->buffer[index] = make_vgaentry(c, color);
}

void ConsoleVGA::increment(int step)
{
	this->column += step;
	if (this->column >= VGA_WIDTH)
	{
		newline();
	}
}
void ConsoleVGA::newline()
{
	this->column = 0;
	if (++this->row == VGA_HEIGHT)
	{
		int total = VGA_WIDTH * (VGA_HEIGHT-1);
		
		for (int n = 0; n < total; n++)
		{
			this->buffer[n] = this->buffer[n + VGA_WIDTH];
		}
		for (int n = 0; n < VGA_WIDTH; n++)
		{
			this->buffer[total + n] = make_vgaentry(' ', color);
		}
		this->row--;
	}
}
void ConsoleVGA::clear()
{
	for (size_t y = 0; y < VGA_HEIGHT; y++)
	for (size_t x = 0; x < VGA_WIDTH;  x++)
	{
		const size_t index = y * VGA_WIDTH + x;
		this->buffer[index] = make_vgaentry(' ', this->color);
	}
}

void ConsoleVGA::write(char c)
{
	static const char NEWLINE   = '\n';
	static const char TABULATOR = '\t';
	static const char SPACE     = ' ';
	
	/*if (c == SPACE)
	{
		increment(1);
	}
	else*/ if (c == TABULATOR)
	{
		increment(4);
	}
	else if (c == NEWLINE)
	{
		newline();
	}
	else
	{
		putEntryAt(c, this->color, this->column, this->row);
		increment(1);
	}
}

void ConsoleVGA::write(const char* data, int len)
{
	for (int i = 0; i < len; i++)
		write(data[i]);
}
