#include <string>

/**
 * This header is for instantiating and implementing
 * missing functionality gluing libc++ to the kernel
 * 
**/

#include <support/newlib/xlocale.h>
#include <time.h>
#include <stdio.h>
#include <sys/reent.h>

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
  
  /// IMPLEMENTATION OF Newlib I/O:
  struct _reent stuff _REENT_INIT(stuff);
  
  #undef stdin
  #undef stdout
  #undef stderr
  // why can't these files be located in c_abi.c? linker issue
  __FILE* stdin;
  __FILE* stdout;
  __FILE* stderr;
  
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
    return 'f';
  }
}

// patch over newlibs lack of locale support
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

int isalnum_l(int c, locale_t) {
  return isalnum(c);
}

int isalpha_l(int c, locale_t) {
  return isalpha(c);
}

int isblank_l(int c, locale_t) {
  return isblank(c);
}

int iscntrl_l(int c, locale_t) {
  return iscntrl(c);
}

int isdigit_l(int c, locale_t) {
  return isdigit(c);
}

int isgraph_l(int c, locale_t) {
  return isgraph(c);
}

int islower_l(int c, locale_t) {
  return islower(c);
}

int isprint_l(int c, locale_t) {
  return isprint(c);
}

int ispunct_l(int c, locale_t) {
  return ispunct(c);
}

int isspace_l(int c, locale_t) {
  return isspace(c);
}

int isupper_l(int c, locale_t) {
  return isupper(c);
}

int isxdigit_l(int c, locale_t) {
  return isxdigit(c);
}

int iswalnum_l(wint_t c, locale_t) {
  return iswalnum(c);
}

int iswalpha_l(wint_t c, locale_t) {
  return iswalpha(c);
}

int iswblank_l(wint_t c, locale_t) {
  return iswblank(c);
}

int iswcntrl_l(wint_t c, locale_t) {
  return iswcntrl(c);
}

int iswdigit_l(wint_t c, locale_t) {
  return iswdigit(c);
}

int iswgraph_l(wint_t c, locale_t) {
  return iswgraph(c);
}

int iswlower_l(wint_t c, locale_t) {
  return iswlower(c);
}

int iswprint_l(wint_t c, locale_t) {
  return iswprint(c);
}

int iswpunct_l(wint_t c, locale_t) {
  return iswpunct(c);
}

int iswspace_l(wint_t c, locale_t) {
  return iswspace(c);
}

int iswupper_l(wint_t c, locale_t) {
  return iswupper(c);
}

int iswxdigit_l(wint_t c, locale_t) {
  return iswxdigit(c);
}

int toupper_l(int c, locale_t) {
  return toupper(c);
}

int tolower_l(int c, locale_t) {
  return tolower(c);
}

int towupper_l(int c, locale_t) {
  return towupper(c);
}

int towlower_l(int c, locale_t) {
  return towlower(c);
}

int strcoll_l(const char *s1, const char *s2, locale_t) {
  return strcoll(s1, s2);
}

size_t strxfrm_l(char *dest, const char *src, size_t n,
                               locale_t) {
  return strxfrm(dest, src, n);
}

size_t strftime_l(char *s, size_t max, const char *format,
                                const struct tm *tm, locale_t) {
  return strftime(s, max, format, tm);
}

int wcscoll_l(const wchar_t *ws1, const wchar_t *ws2, locale_t) {
  return wcscoll(ws1, ws2);
}

size_t wcsxfrm_l(wchar_t *dest, const wchar_t *src, size_t n,
                               locale_t) {
  return wcsxfrm(dest, src, n);
}

long double strtold_l(const char *nptr, char **endptr, locale_t) {
  return strtold(nptr, endptr);
}

long long strtoll_l(const char *nptr, char **endptr, int base, locale_t)
{
  return strtoll(nptr, endptr, base);
}

unsigned long long strtoull_l(const char *nptr, char **endptr, int base, locale_t)
{
  return strtoull(nptr, endptr, base);
}

long long wcstoll_l(const wchar_t *nptr, wchar_t **endptr, int base, locale_t)
{
  return wcstoll(nptr, endptr, base);
}

unsigned long long wcstoull_l(const wchar_t *nptr, wchar_t **endptr, int base, locale_t)
{
  return wcstoull(nptr, endptr, base);
}

long double wcstold_l(const wchar_t *nptr, wchar_t **endptr, locale_t)
{
  return wcstold(nptr, endptr);
}
