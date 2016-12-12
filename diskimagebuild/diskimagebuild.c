#define _XOPEN_SOURCE 1
#define _XOPEN_SOURCE_EXTENDED 1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ftw.h>
#include <errno.h>

int add_item(const char *path, const struct stat *sb, int flag, struct
FTW *ftwbuf) {
    if(ftwbuf->level > 0) {
        fprintf(stdout, "%s\n", path);
    }
    return 0;
}

int main(int argc, char** argv) {

    if (argc != 2) {
       fprintf(stderr, "Usage:\ndiskimagebuild <path>\n");
       exit(1);
    }
    int res = nftw(argv[1], add_item, 256, FTW_PHYS);
    if (res == -1) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(1);
    }

    exit(0);
}