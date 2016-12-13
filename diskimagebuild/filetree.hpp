#pragma once
#include <memory>
#include <cstdint>
#include <vector>

inline uint32_t po2rup(uint32_t x)
{
  x--;
  x |= x >> 1;  //   2 bit numbers
  x |= x >> 2;  //   4 bit numbers
  x |= x >> 4;  //   8 bit numbers
  x |= x >> 8;  //  16 bit numbers
  x |= x >> 16; //  32 bit numbers
  x++;
  return x;
}
template <int Mult = 512>
inline int round_up(int num)
{
  return (num + Mult - 1) & ~(Mult - 1);
}


struct Dir
{
  Dir(const char* path);

  void print(int level);
  
  std::string      name;
  std::vector<Dir> subs;
  std::vector<int> files;
};

struct File
{
  File(const char* path);
  
  uint32_t sectors_used() const
  {
    return round_up<512> (this->size) / 512;
  }
  
  std::string name;
  uint32_t    size;
  std::unique_ptr<char[]> data;
};


struct FileSys
{
  void print();
  void add(const char* path);
  
private:
  Dir root {""};
};
