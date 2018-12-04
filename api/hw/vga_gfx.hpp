#pragma once
#include <cstdint>
#include <cstddef>

struct VGA_gfx
{
  enum modes_t {
    MODE_320_200_256,
    MODE_640_480_16,
  };
  static void set_mode(modes_t mode);

  static int width() { return m_width; }
  static int height() { return m_height; }
  static int bits()  { return m_bits; }

  static void set_palette(const uint32_t colors[256]);
  static void set_palette(const uint8_t idx, int r, int g, int b);
  static void set_pal24(const uint8_t idx, const uint32_t);
  static void apply_default_palette();

  // clears screen fast with given color
  static void clear(uint8_t color = 0);
  // blits to screen from SSE-aligned backbuffer
  static void blit_from(const void*);

  static inline void* address() noexcept
  {
    return VGA_gfx::m_address;
  }
  static inline size_t size() noexcept
  {
    return width() * height() * bits() / 8;
  }

private:
  static void write_regs(uint8_t[]);

  static int m_width;
  static int m_height;
  static int m_bits;
  static void* m_address;
};
