#include <stdarg.h>
#include <sys/types.h>
#include <sys/time.h>

int access(const char *pathname, int mode)
{
	return 0;
}
char* getcwd(char *buf, size_t size)
{
	return 0;
}
int fcntl(int fd, int cmd, _VA_LIST_...)
{
	return 0;
}
int fchmod(int fildes, mode_t mode)
{
	return 0;
}
int mkdir(const char *pathname, mode_t mode)
{
	return 0;
}
int rmdir(const char *pathname)
{
	return 0;
}

int gettimeofday(struct timeval *__restrict tv,
				 void *__restrict tz)
{
	return 0;
}
int settimeofday(const struct timeval *tv, const struct timezone *tz)
{
	return 0;
}
