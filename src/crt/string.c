#include "string.h"

#include <malloc.h>
#include <string.h>

char* strdup(const char* string)
{
	size_t len = strlen(string) * sizeof(char);
	char* dup = (char*) malloc(len);
	memcpy(dup, string, len);
	return dup;
}
