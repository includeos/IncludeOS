#include <common.cxx>
#include <util/membitmap.hpp>

CASE( "Empty chunk data means no set bits" )
{
  uint32_t data = 0;
  MemBitmap bmp(&data, sizeof(data) / sizeof(MemBitmap::word));
  
  EXPECT( bmp.first_set() == -1 );
  EXPECT( bmp.first_free() == 0 );
}
CASE( "All bits set in 32-bits of chunk data" )
{
  uint32_t data = UINT32_MAX;
  MemBitmap bmp(&data, sizeof(data) / sizeof(MemBitmap::word));
  
  EXPECT( bmp.first_set() ==  0 );
  EXPECT( bmp.last_set()  == 31 );
  EXPECT( bmp.first_free() == -1);
}
CASE( "Set and verify each individual bit in 32-bits chunk" )
{
  uint32_t data = 0;
  MemBitmap bmp(&data, sizeof(data) / sizeof(MemBitmap::word));
  
  for (int i = 0; i < 32; i++) {
    bmp.set(i);
    EXPECT( bmp.get(i) == true );
    EXPECT( bmp.last_set() == i );
  }
}
CASE( "Reset and verify each individual bit in 32-bits chunk" )
{
  uint32_t data = UINT32_MAX;
  MemBitmap bmp(&data, sizeof(data) / sizeof(MemBitmap::word));
  
  for (int i = 0; i < 32; i++) {
    EXPECT( bmp.first_set() == i);
    bmp.reset(i);
    EXPECT( bmp.get(i) == false );
  }
}
CASE( "Verify that the index operator matches get function" )
{
  uint32_t data = 0x12345678;
  MemBitmap bmp(&data, sizeof(data) / sizeof(MemBitmap::word));
  
  for (int i = 0; i < 32; i++) {
    EXPECT( bmp.get(i) == bmp[i]);
  }
}
CASE( "Verify zeroing the chunk data" )
{
  uint32_t data = 0x12345678;
  MemBitmap bmp(&data, sizeof(data) / sizeof(MemBitmap::word));
  
  bmp.zero_all();
  EXPECT( bmp.first_set() == -1 );
  EXPECT( bmp.last_set() == -1 );
}
CASE( "Verify setting all bits in chunk data" )
{
  uint32_t data = 0x12345678;
  MemBitmap bmp(&data, sizeof(data) / sizeof(MemBitmap::word));
  
  bmp.set_all();
  EXPECT( bmp.first_set() == 0 );
  EXPECT( bmp.last_set() == 31 );
  for (int i = 0; i < 32; i++) {
    EXPECT( bmp.get(i) == true );
  }
}
CASE( "Verify flipping bits work" )
{
  uint32_t data = 0;
  MemBitmap bmp(&data, sizeof(data) / sizeof(MemBitmap::word));
  
  for (int i = 0; i < 32; i++) {
    bmp.flip(i);
    EXPECT( bmp.get(i) == true );
    bmp.flip(i);
    EXPECT( bmp.get(i) == false );
  }
}
CASE( "Verify setting data location works" )
{
  uint32_t data = 0;
  MemBitmap bmp;
  bmp.set_location(&data, sizeof(data) / sizeof(MemBitmap::word));
  
  EXPECT( bmp.data() == (char*) &data );
  EXPECT( bmp.size() == sizeof(data) );
}
CASE( "Verify size and data for multiple chunks" )
{
  uint32_t data[16];
  MemBitmap bmp(data, sizeof(data) / sizeof(MemBitmap::word));
  
  EXPECT( bmp.data() == (char*) &data );
  EXPECT( bmp.size() == sizeof(data) );
}

CASE( "Bit-and two bitmaps together" )
{
  uint32_t data1[2] = { 0xFFFFFFFF, 0x0 };
  uint32_t data2[2] = { 0xFFFFFFFF, 0xFFFFFFFF };
  uint32_t data3[2];
  
  MemBitmap bmp1(data1, sizeof(data1) / sizeof(MemBitmap::word));
  MemBitmap bmp2(data2, sizeof(data2) / sizeof(MemBitmap::word));
  MemBitmap bmp3(data3, sizeof(data3) / sizeof(MemBitmap::word));
  
  bmp3.set_from_and(bmp1, bmp2);
  EXPECT( bmp3.get_chunk(0) == 0xFFFFFFFF );
  EXPECT( bmp3.get_chunk(1) == 0x0 );
  
  bmp2 &= bmp1;
  EXPECT( bmp2.get_chunk(0) == 0xFFFFFFFF );
  EXPECT( bmp2.get_chunk(1) == 0x0 );
}
