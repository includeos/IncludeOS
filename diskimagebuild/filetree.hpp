#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

#define SECT_SIZE      512
#define SHORTNAME_LEN  sizeof(cl_dir::shortname)

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

struct FileSys;

struct File
{
  File(const char* path);

  uint32_t sectors_used() const
  {
    return round_up<SECT_SIZE> (this->size) / SECT_SIZE;
  }

  long write(FileSys&, FILE*, long) const;

  std::string name;
  uint32_t    size;
  std::unique_ptr<char[]> data;
  size_t size_helper;
  size_t idx_helper;
};

struct Dir
{
  Dir(const char* path);

  Dir&  add_dir (const char* path)
  {
    subs.emplace_back(path);
    return subs.back();
  }
  File& add_file(const char* path)
  {
    files.emplace_back(path);
    return files.back();
  }

  void print(int level) const;

  // recursively count sectors used
  uint32_t sectors_used() const;

  // recursively write dirent
  long write(FileSys&, FILE*, long, long);

  std::string       name;
  std::vector<Dir>  subs;
  std::vector<File> files;
  size_t size_helper;
  size_t idx_helper;
};


struct FileSys
{
  void gather(const char* path = "");

  void print() const;

  // writes the filesystem to a file, returning
  // total bytes written
  long write(FILE*);

  long to_cluster_hi(long pos) const;
  long to_cluster_lo(long pos) const;

private:
  void add_dir(Dir& dvec);
  Dir  root {""};
};
