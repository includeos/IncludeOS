#ifndef FTW_H
#define FTW_H

#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

struct FTW {
  int base;
  int level;
};

#define FTW_F 0 // Non-directory file.
#define FTW_D 1 // Directory.
#define FTW_DNR 2 // Directory without read permission.
#define FTW_DP 3 // Directory with subdirectories visited.
#define FTW_NS 4 // Unknown type; stat() failed.
#define FTW_SL 5 // Symbolic link.
#define FTW_SLN 6 // Symbolic link that names a nonexistent file.


#define FTW_PHYS 0x01
#define FTW_MOUNT 0x02
#define FTW_DEPTH 0x04
#define FTW_CHDIR 0x08

typedef int Nftw_func(const char *, const struct stat *, int, struct FTW *);
//int	nftw(const char *, int (*)(const char *, const struct stat *, int, struct FTW *), int, int);
int	nftw(const char *path, Nftw_func fn, int fd_limit, int flags);

#ifdef __cplusplus
}
#endif

#endif
