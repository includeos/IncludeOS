#ifndef SYS_MMAN_H
#define SYS_MMAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
typedef _off_t off_t;

void *mmap(void* addr, size_t length, 
            int prot,  int flags,
            int fd,    off_t offset);
int munmap(void* addr, size_t length);

#ifdef __cplusplus
}
#endif

#endif
