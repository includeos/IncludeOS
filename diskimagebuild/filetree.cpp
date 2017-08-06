#include "filetree.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <libgen.h>
#include <sys/stat.h>
#include <dirent.h>

File::File(const char* path)
{
  this->name = std::string(path);
  
  FILE* f = fopen(path, "rb");
  assert(f);
  
  fseek(f, 0, SEEK_END);
  this->size = ftell(f);
  rewind(f);

  this->data = std::unique_ptr<char[]> (new char[size], std::default_delete<char[]> ());
  fread(this->data.get(), this->size, 1, f);
  fclose(f);
}
Dir::Dir(const char* path)
{
  this->name = std::string(path);
  
  /// ... ///
}

void Dir::print(int level) const
{
  for (const Dir& d : subs)
  {
    printf ("Dir%*s ", level * 2, "");
    printf("[%u entries] %s\n",
          d.sectors_used(),
          d.name.c_str());
    
    d.print(level + 1);
  }
  for (const File& f : files)
  {
    printf("File%*s ", level * 2, "");
    printf("[%08u b -> %06u sect] %s\n",
          f.size,
          f.sectors_used(),
          f.name.c_str());
  }
}

uint32_t Dir::sectors_used() const
{
  uint32_t cnt = this->size_helper;
  
  for (const auto& dir : subs)
      cnt += dir.sectors_used();
  
  for (const auto& file : files)
      cnt += file.sectors_used();
  
  return cnt;
}

void FileSys::print() const
{
  root.print(0);
}
void FileSys::add_dir(Dir& dvec)
{
  // push work dir
  char pwd_buffer[256];
  getcwd(pwd_buffer, sizeof(pwd_buffer));
  // go into directory
  char cwd_buffer[256];
  getcwd(cwd_buffer, sizeof(cwd_buffer));
  strcat(cwd_buffer, "/");
  strcat(cwd_buffer, dvec.name.c_str());
  
  //printf("*** Entering %s...\n", cwd_buffer);
  chdir(cwd_buffer);
  
  auto* dir = opendir(cwd_buffer);
  if (dir == nullptr)
  {
    printf("Could not open directory:\n-> %s\n", cwd_buffer);
    return;
  }
  struct dirent* ent;
  while ((ent = readdir(dir)) != nullptr)
  {
    std::string name(ent->d_name);
    if (name == ".." || name == ".") continue;
    
    if (ent->d_type == DT_DIR) {
      auto& d = dvec.add_dir(ent->d_name);
      add_dir(d);
    }
    else {
      dvec.add_file(ent->d_name);
    }
  }
  // pop work dir
  chdir(pwd_buffer);
}

void FileSys::gather(const char* path)
{
  root = Dir(path);
  add_dir(root);
}
