#include <string>

/**
 * This header is for instantiating and implementing
 * missing functionality gluing libc++ to the kernel
 * 
**/
#include <time.h>
#include <support/newlib/xlocale.h>
#include <stdio.h>

// needed for <iostream>
extern "C"
{
  /// Linux standard base (locale)
  size_t __ctype_get_mb_cur_max(void)
  {
    return (size_t) 2; // ???
  }
  size_t __mbrlen (const char*, size_t, mbstate_t*)
  {
    printf("WARNING: mbrlen was called - which we don't currently support - returning bogus value\n");
    return (size_t) 1;
  }
  
  typedef int nl_catd;
  
  //nl_catd catopen (const char *cat_name, int flag)
  nl_catd catopen (const char* f, int flag)
  {
    printf("catopen: %s, flag=%d\n", f, flag);
    
    return (nl_catd) -1;
  }
  //char * catgets (nl_catd catalog_desc, int set, int message, const char *string)
  char * catgets (nl_catd, int, int, const char*)
  {
    return NULL;
  }
  //int catclose (nl_catd catalog_desc)
  int catclose (nl_catd)
  {
    return (nl_catd) 0;
  }
  
  char _IO_getc()
  {
    /// NOTE: IMPLEMENT ME
    printf("_IO_getc(): returning bogus character as input from stdin\n");
    return 'f';
  }
}

// patch over newlibs lack of locale support
/*
typedef void *locale_t;

locale_t duplocale(locale_t)
{
  return NULL;
}
locale_t newlocale(int, const char *, locale_t)
{
  return NULL;
}
void freelocale(locale_t) {}

locale_t uselocale(locale_t)
{
  return NULL;
}
*/
