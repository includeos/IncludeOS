#include <sys/types.h>
#include <errno.h>
#include <cstdio>

struct DIR
{
  ino_t  d_ino;       // file serial number
  char   d_name[];    // name of entry
};

struct dirent {
    ino_t          d_ino;       /* inode number */
    off_t          d_off;       /* offset to the next dirent */
    unsigned short d_reclen;    /* length of this record */
    unsigned char  d_type;      /* type of file; not supported
                                   by all file system types */
    char           d_name[256]; /* filename */
};

extern "C"
DIR* opendir(const char *dirname)
{
  printf("opendir(%s)\n", dirname);
  errno = ENOENT;
  return nullptr;
}
extern "C"
struct dirent* readdir(DIR *dirp)
{
  printf("readdir(%p)\n", dirp);
  errno = EINVAL;
  return nullptr;
}
extern "C"
int closedir(DIR *dirp)
{
  return 0;
}
