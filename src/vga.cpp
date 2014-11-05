#include <vga.hpp>

#include <string.h>
#include <x86intrin.h>

ConsoleVGA consoleVGA;

uint16_t make_vgaentry(char c, uint8_t color)
{
	uint16_t c16 = c;
	uint16_t color16 = color;
	return c16 | color16 << 8;
}

ConsoleVGA::ConsoleVGA()
{
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
		this->row--;
		
		unsigned total = VGA_WIDTH * (VGA_HEIGHT-1);
		__m128i scan;
		
		// copy rows upwards
		for (unsigned n = 0; n < total; n += 8)
		{
			scan = _mm_load_si128((__m128i*) &buffer[n + VGA_WIDTH]);
			_mm_store_si128((__m128i*) &buffer[n], scan);
		}
		
		// clear out the last row
		scan = _mm_set1_epi16(make_vgaentry(' ', this->color));
		
		for (unsigned n = 0; n < VGA_WIDTH; n += 8)
		{
			_mm_store_si128((__m128i*) &buffer[total + n], scan);
		}
	}
}
void ConsoleVGA::clear()
{
	this->row    = 0;
	this->column = 0;
	
	__m128i scan = _mm_set1_epi16(make_vgaentry(' ', this->color));
	
	for (size_t y = 0; y < VGA_HEIGHT; y++)
	for (size_t x = 0; x < VGA_WIDTH;  x += 8)
	{
		const size_t index = y * VGA_WIDTH + x;
		_mm_store_si128((__m128i*) &buffer[index], scan);
	}
}

void ConsoleVGA::write(char c)
{
	static const char NEWLINE   = '\n';
	static const char TABULATOR = '\t';
	//static const char SPACE     = ' ';
	
	if (c == TABULATOR)
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
