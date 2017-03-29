#include <clocale>
#include <malloc.h>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <reent.h>

locale_t __c_locale = nullptr;

extern "C"
void freelocale(locale_t loc)
{
  if ((void*) loc != (void*) __c_locale)
    free((void*) loc);
}

extern "C"
locale_t newlocale(int mask, const char* locale, locale_t base)
{
  (void)mask;
  (void)base;

  if ((locale == NULL) || (locale[0] == '\0') ||
      ((locale[0] == 'C') && (locale[1] == '\0')) ||
      ((locale[0] == 'P') && (locale[1] == 'O') && (locale[2] == 'S') &&
       (locale[3] == 'I') && (locale[4] == 'X') && (locale[5] == '\0'))) {
    return (locale_t) __c_locale;
  }
  return 0;
}

extern "C"
locale_t uselocale(locale_t new_locale)
{
  locale_t old_locale = (locale_t) _setlocale_r(_REENT, LC_ALL, nullptr);
  if (new_locale == nullptr) {
      return old_locale;
  }
  setlocale(LC_ALL, (const char*) new_locale);
  return old_locale;
}

__attribute__ ((constructor))
static void init_c_locale()
{
  __c_locale = (locale_t) setlocale(LC_ALL, "C");
}
