#pragma once
#include <memory>
#include <cstdint>
#include <vector>

#define SECT_SIZE   512

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
template <int Mult = SECT_SIZE>
inline int round_up(int num)
{
  return (num + Mult - 1) & ~(Mult - 1);
}


struct File
{
  File(const char* path);
  
  uint32_t sectors_used() const
  {
    return round_up<SECT_SIZE> (this->size) / SECT_SIZE;
  }
  
  std::string name;
  uint32_t    size;
  std::unique_ptr<char[]> data;
};

struct Dir
{
  Dir(const char* path);

  void print(int level) const;
  
  // recursively count sectors used
  uint32_t sectors_used() const;
  
  // recursively write dirent
  void write(FILE*, long);
  
  std::string       name;
  std::vector<Dir>  subs;
  std::vector<File> files;
};


struct FileSys
{
  void print() const;
  void add(const char* path);
  
  void write(const char* path);
  
private:
  Dir root {""};
};
