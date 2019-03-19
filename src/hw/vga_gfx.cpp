#include <hw/vga_gfx.hpp>
#include <hw/ioport.hpp>
#if defined(__SSE2__)
  #include <x86intrin.h>
#endif
#include <cassert>
int   VGA_gfx::m_width  = 0;
int   VGA_gfx::m_height = 0;
int   VGA_gfx::m_bits   = 0;
void* VGA_gfx::m_address = nullptr;

// Chris Giese <geezer@execpc.com>	http://my.execpc.com/~geezer
// https://files.osdev.org/mirrors/geezer/osd/graphics/modes.c
#define	VGA_AC_INDEX		0x3C0
#define	VGA_AC_WRITE		0x3C0
#define	VGA_AC_READ		0x3C1
#define	VGA_MISC_WRITE		0x3C2
#define VGA_SEQ_INDEX		0x3C4
#define VGA_SEQ_DATA		0x3C5
#define	VGA_DAC_READ_INDEX	0x3C7
#define	VGA_DAC_WRITE_INDEX	0x3C8
#define	VGA_DAC_DATA		0x3C9
#define	VGA_MISC_READ		0x3CC
#define VGA_GC_INDEX 		0x3CE
#define VGA_GC_DATA 		0x3CF
/*			COLOR emulation		MONO emulation */
#define VGA_CRTC_INDEX		0x3D4		/* 0x3B4 */
#define VGA_CRTC_DATA		0x3D5		/* 0x3B5 */
#define	VGA_INSTAT_READ		0x3DA

#define	VGA_NUM_SEQ_REGS	 5
#define	VGA_NUM_CRTC_REGS	25
#define	VGA_NUM_GC_REGS		 9
#define	VGA_NUM_AC_REGS		21
#define	VGA_NUM_REGS		(1 + VGA_NUM_SEQ_REGS + VGA_NUM_CRTC_REGS + \
				                VGA_NUM_GC_REGS + VGA_NUM_AC_REGS)

static uint8_t g_320x200x256[] =
{
/* MISC */
	0x63,
/* SEQ */
	0x03, 0x01, 0x0F, 0x00, 0x0E,
/* CRTC */
	0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
	0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x9C, 0x0E, 0x8F, 0x28,	0x40, 0x96, 0xB9, 0xA3,
	0xFF,
/* GC */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
	0xFF,
/* AC */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x41, 0x00, 0x0F, 0x00,	0x00
};
static uint8_t g_640x480x16[] =
{
/* MISC */
	0xE3,
/* SEQ */
	0x03, 0x01, 0x08, 0x00, 0x06,
/* CRTC */
	0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xEA, 0x0C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xE3,
	0xFF,
/* GC */
	0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x05, 0x0F,
	0xFF,
/* AC */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x01, 0x00, 0x0F, 0x00, 0x00
};

void VGA_gfx::write_regs(uint8_t regs[])
{
  /* write MISCELLANEOUS reg */
  hw::outb(VGA_MISC_WRITE, regs[0]);
  /* write SEQUENCER regs */
  for (int i = 0; i < VGA_NUM_SEQ_REGS; i++)
  {
    hw::outb(VGA_SEQ_INDEX, i);
    hw::outb(VGA_SEQ_DATA, regs[1 + i]);
  }
  /* unlock CRTC registers */
  hw::outb(VGA_CRTC_INDEX, 0x03);
  hw::outb(VGA_CRTC_DATA, hw::inb(VGA_CRTC_DATA) | 0x80);
  hw::outb(VGA_CRTC_INDEX, 0x11);
  hw::outb(VGA_CRTC_DATA, hw::inb(VGA_CRTC_DATA) & ~0x80);
  /* make sure they remain unlocked */
  regs[0x03] |= 0x80;
  regs[0x11] &= ~0x80;
  /* write CRTC regs */
  for (int i = 0; i < VGA_NUM_CRTC_REGS; i++)
  {
    hw::outb(VGA_CRTC_INDEX, i);
    hw::outb(VGA_CRTC_DATA, regs[6 + i]);
  }
  /* write GRAPHICS CONTROLLER regs */
  for (int i = 0; i < VGA_NUM_GC_REGS; i++)
  {
    hw::outb(VGA_GC_INDEX, i);
    hw::outb(VGA_GC_DATA, regs[31 + i]);
  }
  /* write ATTRIBUTE CONTROLLER regs */
  for (int i = 0; i < VGA_NUM_AC_REGS; i++)
  {
    (void) hw::inb(VGA_INSTAT_READ);
    hw::outb(VGA_AC_INDEX, i);
    hw::outb(VGA_AC_WRITE, regs[40 + i]);
  }

  /* lock 16-color palette and unblank display */
  (void) hw::inb(VGA_INSTAT_READ);
  hw::outb(VGA_AC_INDEX, 0x20);
}

void VGA_gfx::set_mode(modes_t mode)
{
  switch (mode)
  {
  case MODE_320_200_256:
    VGA_gfx::m_width  = 320;
    VGA_gfx::m_height = 200;
    VGA_gfx::m_bits   = 8;
    VGA_gfx::m_address = (void*) 0xA0000;
    write_regs(g_320x200x256);
    break;
  case MODE_640_480_16:
    VGA_gfx::m_width  = 640;
    VGA_gfx::m_height = 480;
    VGA_gfx::m_bits   = 4;
    VGA_gfx::m_address = (void*) 0xA0000;
    write_regs(g_640x480x16);
    break;
  }
}

void VGA_gfx::set_palette(const uint32_t colors[256])
{
  hw::outb(0x03c6, 0xff);
  hw::outb(0x03c8, 0x0);
  for (int c = 0; c < 256; c++)
  {
    hw::outb(0x3c9, (colors[c] >>  2) & 0x3F);
    hw::outb(0x3c9, (colors[c] >> 10) & 0x3F);
    hw::outb(0x3c9, (colors[c] >> 18) & 0x3F);
  }
}
void VGA_gfx::set_palette(const uint8_t idx, int r, int g, int b)
{
	// select color index
	hw::outb(0x03c8, idx);
	// write 18-bit color
  hw::outb(0x3c9, r);
  hw::outb(0x3c9, g);
  hw::outb(0x3c9, b);
}
void VGA_gfx::set_pal24(const uint8_t idx, const uint32_t color)
{
	// select color index
	hw::outb(0x03c8, idx);
	// write 18-bit color
	hw::outb(0x3c9, (color >>  2) & 0x3F);
	hw::outb(0x3c9, (color >> 10) & 0x3F);
	hw::outb(0x3c9, (color >> 18) & 0x3F);
}

void VGA_gfx::apply_default_palette()
{
  #define rgb(r, g, b) (r | (g << 8) | (b << 16))
  const uint32_t palette[256] =
  {
    rgb(  0,  0,  0),
    rgb(  0,  0,170),
    rgb(  0,170,  0),
    rgb(  0,170,170),
    rgb(170,  0,  0),
    rgb(170,  0,170),
    rgb(170, 85,  0),
    rgb(170,170,170),
    rgb( 85, 85, 85),
    rgb( 85, 85,255),
    rgb( 85,255, 85),
    rgb( 85,255,255),
    rgb(255, 85, 85),
    rgb(255, 85,255),
    rgb(255,255, 85),
    rgb(255,255,255),
    rgb(  0,  0,  0),
    rgb( 20, 20, 20),
    rgb( 32, 32, 32),
    rgb( 44, 44, 44),
    rgb( 56, 56, 56),
    rgb( 68, 68, 68),
    rgb( 80, 80, 80),
    rgb( 97, 97, 97),
    rgb(113,113,113),
    rgb(129,129,129),
    rgb(145,145,145),
    rgb(161,161,161),
    rgb(182,182,182),
    rgb(202,202,202),
    rgb(226,226,226),
    rgb(255,255,255),
    rgb(  0,  0,255),
    rgb( 64,  0,255),
    rgb(125,  0,255),
    rgb(190,  0,255),
    rgb(255,  0,255),
    rgb(255,  0,190),
    rgb(255,  0,125),
    rgb(255,  0, 64),
    rgb(255,  0,  0),
    rgb(255, 64,  0),
    rgb(255,125,  0),
    rgb(255,190,  0),
    rgb(255,255,  0),
    rgb(190,255,  0),
    rgb(125,255,  0),
    rgb( 64,255,  0),
    rgb(  0,255,  0),
    rgb(  0,255, 64),
    rgb(  0,255,125),
    rgb(  0,255,190),
    rgb(  0,255,255),
    rgb(  0,190,255),
    rgb(  0,125,255),
    rgb(  0, 64,255),
    rgb(125,125,255),
    rgb(157,125,255),
    rgb(190,125,255),
    rgb(222,125,255),
    rgb(255,125,255),
    rgb(255,125,222),
    rgb(255,125,190),
    rgb(255,125,157),
    rgb(255,125,125),
    rgb(255,157,125),
    rgb(255,190,125),
    rgb(255,222,125),
    rgb(255,255,125),
    rgb(222,255,125),
    rgb(190,255,125),
    rgb(157,255,125),
    rgb(125,255,125),
    rgb(125,255,157),
    rgb(125,255,190),
    rgb(125,255,222),
    rgb(125,255,255),
    rgb(125,222,255),
    rgb(125,190,255),
    rgb(125,157,255),
    rgb(182,182,255),
    rgb(198,182,255),
    rgb(218,182,255),
    rgb(234,182,255),
    rgb(255,182,255),
    rgb(255,182,234),
    rgb(255,182,218),
    rgb(255,182,198),
    rgb(255,182,182),
    rgb(255,198,182),
    rgb(255,218,182),
    rgb(255,234,182),
    rgb(255,255,182),
    rgb(234,255,182),
    rgb(218,255,182),
    rgb(198,255,182),
    rgb(182,255,182),
    rgb(182,255,198),
    rgb(182,255,218),
    rgb(182,255,234),
    rgb(182,255,255),
    rgb(182,234,255),
    rgb(182,218,255),
    rgb(182,198,255),
    rgb(  0,  0,113),
    rgb( 28,  0,113),
    rgb( 56,  0,113),
    rgb( 85,  0,113),
    rgb(113,  0,113),
    rgb(113,  0, 85),
    rgb(113,  0, 56),
    rgb(113,  0, 28),
    rgb(113,  0,  0),
    rgb(113, 28,  0),
    rgb(113, 56,  0),
    rgb(113, 85,  0),
    rgb(113,113,  0),
    rgb( 85,113,  0),
    rgb( 56,113,  0),
    rgb( 28,113,  0),
    rgb(  0,113,  0),
    rgb(  0,113, 28),
    rgb(  0,113, 56),
    rgb(  0,113, 85),
    rgb(  0,113,113),
    rgb(  0, 85,113),
    rgb(  0, 56,113),
    rgb(  0, 28,113),
    rgb( 56, 56,113),
    rgb( 68, 56,113),
    rgb( 85, 56,113),
    rgb( 97, 56,113),
    rgb(113, 56,113),
    rgb(113, 56, 97),
    rgb(113, 56, 85),
    rgb(113, 56, 68),
    rgb(113, 56, 56),
    rgb(113, 68, 56),
    rgb(113, 85, 56),
    rgb(113, 97, 56),
    rgb(113,113, 56),
    rgb( 97,113, 56),
    rgb( 85,113, 56),
    rgb( 68,113, 56),
    rgb( 56,113, 56),
    rgb( 56,113, 68),
    rgb( 56,113, 85),
    rgb( 56,113, 97),
    rgb( 56,113,113),
    rgb( 56, 97,113),
    rgb( 56, 85,113),
    rgb( 56, 68,113),
    rgb( 80, 80,113),
    rgb( 89, 80,113),
    rgb( 97, 80,113),
    rgb(105, 80,113),
    rgb(113, 80,113),
    rgb(113, 80,105),
    rgb(113, 80, 97),
    rgb(113, 80, 89),
    rgb(113, 80, 80),
    rgb(113, 89, 80),
    rgb(113, 97, 80),
    rgb(113,105, 80),
    rgb(113,113, 80),
    rgb(105,113, 80),
    rgb( 97,113, 80),
    rgb( 89,113, 80),
    rgb( 80,113, 80),
    rgb( 80,113, 89),
    rgb( 80,113, 97),
    rgb( 80,113,105),
    rgb( 80,113,113),
    rgb( 80,105,113),
    rgb( 80, 97,113),
    rgb( 80, 89,113),
    rgb(  0,  0, 64),
    rgb( 16,  0, 64),
    rgb( 32,  0, 64),
    rgb( 48,  0, 64),
    rgb( 64,  0, 64),
    rgb( 64,  0, 48),
    rgb( 64,  0, 32),
    rgb( 64,  0, 16),
    rgb( 64,  0,  0),
    rgb( 64, 16,  0),
    rgb( 64, 32,  0),
    rgb( 64, 48,  0),
    rgb( 64, 64,  0),
    rgb( 48, 64,  0),
    rgb( 32, 64,  0),
    rgb( 16, 64,  0),
    rgb(  0, 64,  0),
    rgb(  0, 64, 16),
    rgb(  0, 64, 32),
    rgb(  0, 64, 48),
    rgb(  0, 64, 64),
    rgb(  0, 48, 64),
    rgb(  0, 32, 64),
    rgb(  0, 16, 64),
    rgb( 32, 32, 64),
    rgb( 40, 32, 64),
    rgb( 48, 32, 64),
    rgb( 56, 32, 64),
    rgb( 64, 32, 64),
    rgb( 64, 32, 56),
    rgb( 64, 32, 48),
    rgb( 64, 32, 40),
    rgb( 64, 32, 32),
    rgb( 64, 40, 32),
    rgb( 64, 48, 32),
    rgb( 64, 56, 32),
    rgb( 64, 64, 32),
    rgb( 56, 64, 32),
    rgb( 48, 64, 32),
    rgb( 40, 64, 32),
    rgb( 32, 64, 32),
    rgb( 32, 64, 40),
    rgb( 32, 64, 48),
    rgb( 32, 64, 56),
    rgb( 32, 64, 64),
    rgb( 32, 56, 64),
    rgb( 32, 48, 64),
    rgb( 32, 40, 64),
    rgb( 44, 44, 64),
    rgb( 48, 44, 64),
    rgb( 52, 44, 64),
    rgb( 60, 44, 64),
    rgb( 64, 44, 64),
    rgb( 64, 44, 60),
    rgb( 64, 44, 52),
    rgb( 64, 44, 48),
    rgb( 64, 44, 44),
    rgb( 64, 48, 44),
    rgb( 64, 52, 44),
    rgb( 64, 60, 44),
    rgb( 64, 64, 44),
    rgb( 60, 64, 44),
    rgb( 52, 64, 44),
    rgb( 48, 64, 44),
    rgb( 44, 64, 44),
    rgb( 44, 64, 48),
    rgb( 44, 64, 52),
    rgb( 44, 64, 60),
    rgb( 44, 64, 64),
    rgb( 44, 60, 64),
    rgb( 44, 52, 64),
    rgb( 44, 48, 64),
    rgb(  0,  0,  0),
    rgb(  0,  0,  0),
    rgb(  0,  0,  0),
    rgb(  0,  0,  0),
    rgb(  0,  0,  0),
    rgb(  0,  0,  0),
    rgb(  0,  0,  0),
    rgb(  0,  0,  0)
  };
  VGA_gfx::set_palette(palette);
}

void VGA_gfx::clear(uint8_t clr)
{
#if defined(__SSE2__)
  const auto addr = (__m128i*) VGA_gfx::address();
  const auto end  = addr + VGA_gfx::size() / 16;
  const auto tmp = _mm_set1_epi8(clr);
  for (auto* imm = addr; imm < end; imm++) {
		 _mm_stream_si128(imm, tmp);
	}
#else //dead slow fallback
	const auto addr = (char*) VGA_gfx::address();
	const auto end  = addr + VGA_gfx::size();
	for (auto* imm = addr; imm < end; imm++) {
		*imm=clr;
	}
#endif
}
void VGA_gfx::blit_from(const void* vsrc)
{
#if defined(__SSE2__)
  const auto addr = (__m128i*) VGA_gfx::address();
  const auto end  = addr + VGA_gfx::size() / 16;
  const auto src = (__m128i*) vsrc;

  for (intptr_t i = 0; i < end - addr; i++)
    _mm_stream_si128(&addr[i], src[i]);
#else //dead slow fallback
	const auto addr = (char*) VGA_gfx::address();
	const auto end  = addr + VGA_gfx::size() ;
	const auto src = (char*) vsrc;

	for (intptr_t i = 0; i < end - addr; i++) {
		addr[i]=src[i];
	}
#endif
}
