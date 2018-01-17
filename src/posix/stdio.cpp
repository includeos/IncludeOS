#include <cstdio>
#include <string.h>
#include <cassert>

extern "C"
FILE* fopen64(const char *filename, const char *type)
{
  return fopen(filename, type);
}

extern "C" __attribute__((format(printf, 2, 3)))
int __printf_chk (int flag, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  int done = vfprintf (stdout, format, ap);
  va_end (ap);
  return done;
}
extern "C"
int __fprintf_chk(FILE* fp, int flag, const char* format, ...)
{
  va_list arg;
  va_start (arg, format);
  int done = vfprintf(fp, format, arg);
  va_end (arg);
  return done;
}
extern "C"
int __vfprintf_chk(FILE* fp, int flag, const char *format, va_list ap)
{
  int done;
  done = vfprintf (fp, format, ap);
  return done;
}
extern "C" __attribute__((format(printf, 4, 5)))
int __sprintf_chk(char* s, int flags, size_t slen, const char *format, ...)
{
  va_list arg;
  int done;
  va_start (arg, format);
  done = vsprintf(s, format, arg);
  va_end (arg);
  return done;
}
extern "C"
char* __strcat_chk(char* dest, const char* src, size_t destlen)
{
  size_t len = strlen(dest) + strlen(src);
  assert (len > destlen);
  return strcat(dest, src);
}
extern "C"
int __isoc99_scanf (const char *format, ...)
{
  va_list arg;
  va_start (arg, format);
  int done = vfscanf(stdin, format, arg);
  va_end (arg);
  return done;
}
extern "C" __attribute__((format(scanf, 2, 3)))
int __isoc99_sscanf (const char *s, const char *format, ...)
{
  va_list arg;
  int done;
  va_start (arg, format);
  done = vsscanf(s, format, arg);
  va_end (arg);
  return done;
}
