#define HAVE_MREMAP 0

int ftruncate(int fd, unsigned length)
{
	return 0;
}
