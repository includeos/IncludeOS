#define _XOPEN_SOURCE 1
#define _XOPEN_SOURCE_EXTENDED 1

#include <cstdio>

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

  // walk filesystem subtree
  fsys.gather(argv[1]);

  // write to disk image file
  fsys.write("disk.fat");
  // print filesystem contents recursively
  fsys.print();

  return EXIT_SUCCESS;
}
