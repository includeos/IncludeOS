#pragma once
#include <stdint.h>
#include <x86intrin.h>
#include <iostream>

#define SSE_ALIGNED   alignof(__m128i)

struct TestAddr
{
  // constructors
  TestAddr()
    : i64{0, 0} {}
  TestAddr(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
    : i32{a, b, c, d} {}
  TestAddr(uint64_t top, uint64_t bot)
    : i64{top, bot} {}
  
  // copy-constructor
  TestAddr(const TestAddr& a)
  {
    std::cout << a.to_string() << std::endl;
    
    this->i64[0] = a.i64[0];
    this->i64[1] = a.i64[1];
  }
  // assignment
  TestAddr& operator= (const TestAddr& a)
  {
    this->i64[0] = a.i64[0];
    this->i64[1] = a.i64[1];
    return *this;
  }
  // returns this IPv6 address as a string
  std::string to_string() const;
  
  // fields
  uint64_t i64[2];
  uint32_t i32[4];
  uint16_t i16[8];
  uint8_t  i8[16];
  
}; // __attribute__((aligned(SSE_ALIGNED)));
