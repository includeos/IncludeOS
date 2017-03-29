//===-------------------------- __cxxabi_config.h -------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef ____CXXABI_CONFIG_H
#define ____CXXABI_CONFIG_H

// much better than abort()
void abort_ex(const char*) __attribute__((noreturn));

// Set this in the CXXFLAGS when you need it
#  define LIBCXXABI_HAS_NO_THREADS 1

// Set this in the CXXFLAGS when you need it, because otherwise we'd have to
// #if !defined(__linux__) && !defined(__APPLE__) && ...
// and so-on for *every* platform.
#  define LIBCXXABI_BAREMETAL 1

//#  define _LIBCXX_DYNAMIC_FALLBACK

#if defined(__arm__) && !defined(__USING_SJLJ_EXCEPTIONS__) &&                 \
    !defined(__ARM_DWARF_EH__)
#define LIBCXXABI_ARM_EHABI 1
#else
#define LIBCXXABI_ARM_EHABI 0
#endif

#if !defined(__has_attribute)
#define __has_attribute(_attribute_) 0
#endif

#if defined(_LIBCXXABI_DLL)
 #if defined(cxxabi_EXPORTS)
  #define _LIBCXXABI_HIDDEN
  #define _LIBCXXABI_DATA_VIS __declspec(dllexport)
  #define _LIBCXXABI_FUNC_VIS __declspec(dllexport)
  #define _LIBCXXABI_TYPE_VIS __declspec(dllexport)
 #else
  #define _LIBCXXABI_HIDDEN
  #define _LIBCXXABI_DATA_VIS __declspec(dllimport)
  #define _LIBCXXABI_FUNC_VIS __declspec(dllimport)
  #define _LIBCXXABI_TYPE_VIS __declspec(dllimport)
 #endif
#else
 #define _LIBCXXABI_HIDDEN __attribute__((__visibility__("hidden")))
 #define _LIBCXXABI_DATA_VIS __attribute__((__visibility__("default")))
 #define _LIBCXXABI_FUNC_VIS __attribute__((__visibility__("default")))
 #if __has_attribute(__type_visibility__)
  #define _LIBCXXABI_TYPE_VIS __attribute__((__type_visibility__("default")))
 #else
  #define _LIBCXXABI_TYPE_VIS __attribute__((__visibility__("default")))
 #endif
#endif

#endif // ____CXXABI_CONFIG_H
