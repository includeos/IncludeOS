#define _XOPEN_SOURCE 1
#define _XOPEN_SOURCE_EXTENDED 1

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ftw.h>
#include <errno.h>

#include "filetree.hpp"
FileSys fsys;


int add_item(
      const char*        path, 
      const struct stat* sb, 
      int                flag, 
      struct FTW*        ftwbuf)
{
  if(ftwbuf->level > 0) {
    fsys.add(path);
  }
  return 0;
}

int main(int argc, char** argv)
{
  if (argc != 2) {
     fprintf(stderr, "Usage:\n"
                     "%s [options] file...\n"
                     , argv[0]);
     return EXIT_FAILURE;
  }
  int res = nftw(argv[1], add_item, 256, FTW_PHYS);
  if (res == -1) {
      fprintf(stderr, "%s\n", strerror(errno));
      return EXIT_FAILURE;
  }

  fsys.print();

  return EXIT_SUCCESS;
}
