// -*- C++ -*-
//===------------------- support/xlocale/xlocale.h ------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This is a shared implementation of a shim to provide extended locale support
// on top of libc's that don't support it (like Android's bionic, and Newlib).
//
// The 'illusion' only works when the specified locale is "C" or "POSIX", but
// that's about as good as we can do without implementing full xlocale support
// in the underlying libc.
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP_SUPPORT_XLOCALE_XLOCALE_H
#define _LIBCPP_SUPPORT_XLOCALE_XLOCALE_H

#ifdef __cplusplus
extern "C" {
#endif

// needed for wcstold_l 
long double wcstold (const wchar_t* str, wchar_t** endptr);

// needed for ctype<T>::is(mask, char_type)
int isascii(int ch);
int isalnum_l(int c, locale_t);
int isalpha_l(int c, locale_t);
int isblank_l(int c, locale_t);
int iscntrl_l(int c, locale_t);
int isdigit_l(int c, locale_t);
int isgraph_l(int c, locale_t);
int islower_l(int c, locale_t);
int isprint_l(int c, locale_t);
int ispunct_l(int c, locale_t);
int isspace_l(int c, locale_t);
int isupper_l(int c, locale_t);
int isxdigit_l(int c, locale_t);
int iswalnum_l(wint_t c, locale_t);
int iswalpha_l(wint_t c, locale_t);
int iswblank_l(wint_t c, locale_t);
int iswcntrl_l(wint_t c, locale_t);
int iswdigit_l(wint_t c, locale_t);
int iswgraph_l(wint_t c, locale_t);
int iswlower_l(wint_t c, locale_t);
int iswprint_l(wint_t c, locale_t);
int iswpunct_l(wint_t c, locale_t);
int iswspace_l(wint_t c, locale_t);
int iswupper_l(wint_t c, locale_t);
int iswxdigit_l(wint_t c, locale_t);
int toupper_l(int c, locale_t);
int tolower_l(int c, locale_t);
int towupper_l(int c, locale_t);
int towlower_l(int c, locale_t);
int strcoll_l(const char *s1, const char *s2, locale_t);
size_t strxfrm_l(char *dest, const char *src, size_t n, locale_t);
size_t strftime_l(char *s, size_t max, const char *format, const struct tm *tm, locale_t);
int    wcscoll_l(const wchar_t *ws1, const wchar_t *ws2, locale_t);
size_t wcsxfrm_l(wchar_t *dest, const wchar_t *src, size_t n, locale_t);
long double strtold_l(const char *nptr, char **endptr, locale_t);
long long   strtoll_l(const char *nptr, char **endptr, int base, locale_t);
unsigned long long strtoull_l(const char *nptr, char **endptr, int base, locale_t);
long long          wcstoll_l (const wchar_t *nptr, wchar_t **endptr, int base, locale_t);
unsigned long long wcstoull_l(const wchar_t *nptr, wchar_t **endptr, int base, locale_t);
long double        wcstold_l (const wchar_t *nptr, wchar_t **endptr, locale_t);

#ifdef __cplusplus
}
#endif

#endif // _LIBCPP_SUPPORT_XLOCALE_XLOCALE_H
