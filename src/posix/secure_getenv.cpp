#include <cstdlib>

extern "C"
char* secure_getenv(const char* name)
{
  return getenv(name);
}