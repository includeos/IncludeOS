#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void *dlopen(const char *filename, int flag);
char *dlerror(void);
void *dlsym(void *handle, const char *symbol);
int dlclose(void *handle); 


#define RTLD_LAZY      1
#define RTLD_NOW       2
#define RTLD_GLOBAL    3
#define RTLD_LOCAL     4
#define RTLD_NODELETE  6
#define RTLD_NOLOAD    7
#define RTLD_DEEPBIND  8

#define RTLD_DEFAULT   1

#ifdef __cplusplus
}
#endif
