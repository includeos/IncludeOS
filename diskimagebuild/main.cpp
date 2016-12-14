#define _XOPEN_SOURCE 1
#define _XOPEN_SOURCE_EXTENDED 1

#include <cstdio>
#include <unistd.h>

#include "filetree.hpp"
FileSys fsys;

int main(int argc, char** argv)
{
  if (argc != 2) {
     fprintf(stderr, "Usage:\n"
                     "%s [options] file...\n"
                     , argv[0]);
     return EXIT_FAILURE;
  }

  FILE* file = fopen("disk.fat", "wb");
  chdir(argv[1]);
  // walk filesystem subtree
  fsys.gather();

  // write to disk image file
  fsys.write(file);
  fclose(file);
  
  // print filesystem contents recursively
  fsys.print();

  return EXIT_SUCCESS;
}
