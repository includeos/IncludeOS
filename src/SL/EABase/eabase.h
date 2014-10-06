/*
Copyright (C) 2009 Electronic Arts, Inc.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1.  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
2.  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
3.  Neither the name of Electronic Arts, Inc. ("EA") nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY ELECTRONIC ARTS AND ITS CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ELECTRONIC ARTS OR ITS CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*-----------------------------------------------------------------------------
 * eabase.h
 *
 * Copyright (c) 2002 - 2005 Electronic Arts Inc. All rights reserved.
 * Maintained by Paul Pedriana, Maxis
 *---------------------------------------------------------------------------*/


#ifndef INCLUDED_eabase_H
#define INCLUDED_eabase_H


// Identify the compiler and declare the EA_COMPILER_xxxx defines
#ifndef INCLUDED_eacompiler_H
#  include "EABase/config/eacompiler.h"
#endif

// Identify traits which this compiler supports, or does not support
#ifndef INCLUDED_eacompilertraits_H
#  include "EABase/config/eacompilertraits.h"
#endif

// Identify the platform and declare the EA_xxxx defines
#ifndef INCLUDED_eaplatform_H
#  include "EABase/config/eaplatform.h"
#endif



///////////////////////////////////////////////////////////////////////////////
// EABASE_VERSION
//
// We more or less follow the conventional EA packaging approach to versioning 
// here. A primary distinction here is that minor versions are defined as two
// digit entities (e.g. .03") instead of minimal digit entities ".3"). The logic
// here is that the value is a counter and not a floating point fraction.
// Note that the major version doesn't have leading zeros.
//
// Example version strings:
//      "0.91.00"   // Major version 0, minor version 91, patch version 0. 
//      "1.00.00"   // Major version 1, minor and patch version 0.
//      "3.10.02"   // Major version 3, minor version 10, patch version 02.
//     "12.03.01"   // Major version 12, minor version 03, patch version 
//
// Example usage:
//     printf("EABASE version: %s", EABASE_VERSION);
//     printf("EABASE version: %d.%d.%d", EABASE_VERSION_N / 10000 % 100, EABASE_VERSION_N / 100 % 100, EABASE_VERSION_N % 100);
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EABASE_VERSION
#  define EABASE_VERSION   "2.00.22"
#  define EABASE_VERSION_N  20022
#endif



// ------------------------------------------------------------------------
// The C++ standard defines size_t as a built-in type. Some compilers are
// not standards-compliant in this respect, so we need an additional include.
// The case is similar with wchar_t under C++.

#if defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_MSVC) || defined(EA_WCHAR_T_NON_NATIVE)
#  include <stddef.h>
#endif


// ------------------------------------------------------------------------
// Ensure this header file is only processed once (with certain compilers)
// GCC doesn't need such a pragma because it has special recognition for 
// include guards (such as that above) and effectively implements the same
// thing without having to resort to non-portable pragmas. It is possible
// that the decision to use pragma once here is ill-advised, perhaps because
// some compilers masquerade as MSVC but don't implement all features.
#if defined(EA_COMPILER_MSVC) || defined(EA_COMPILER_METROWERKS)
#  pragma once
#endif


// ------------------------------------------------------------------------
// By default, GCC on certain platforms defines NULL as ((void*)0), which is the
// C definition. This causes all sort of problems for C++ code, so it is
// worked around by undefining NULL.

#if defined(NULL)
#  undef NULL
#endif


// ------------------------------------------------------------------------
// Define the NULL pointer. This is normally defined in <stddef.h>, but we
// don't want to force a global dependency on that header, so the definition
// is duplicated here.

#if defined(__cplusplus)
#  define NULL 0
#else
#  define NULL ((void*)0)
#endif


// ------------------------------------------------------------------------
// C98/99 Standard typedefs. From the ANSI ISO/IEC 9899 standards document
// Most recent versions of the gcc-compiler come with these defined in
// inttypes.h or stddef.h. Determining if they are predefined can be
// tricky, so we expect some problems on non-standard compilers

// ------------------------------------------------------------------------
// We need to test this after we potentially include stddef.h, otherwise we
// would have put this into the compilertraits header.
#if !defined(EA_COMPILER_HAS_INTTYPES) && (!defined(_MSC_VER) || (_MSC_VER > 1500)) && (defined(EA_COMPILER_IS_C99) || defined(INT8_MIN) || defined(EA_COMPILER_HAS_C99_TYPES) || defined(_SN_STDINT_H))
#  define EA_COMPILER_HAS_INTTYPES
#endif


#ifdef EA_COMPILER_HAS_INTTYPES // If the compiler supports inttypes...
    // ------------------------------------------------------------------------
    // Include the stdint header to define and derive the required types.
    // Additionally include inttypes.h as many compilers, including variations
    // of GCC define things in inttypes.h that the C99 standard says goes 
    // in stdint.h.
    //
    // The C99 standard specifies that inttypes.h only define printf/scanf 
    // format macros if __STDC_FORMAT_MACROS is defined before #including
    // inttypes.h. For consistency, we do that here.
#  ifndef __STDC_FORMAT_MACROS
#    define __STDC_FORMAT_MACROS
#  endif
#  if !defined(__psp__) && defined(__GNUC__) // The GCC compiler defines standard int types (e.g. uint32_t) but not PRId8, etc.
#    include <inttypes.h> // PRId8, SCNd8, etc.
#  endif
#  include <stdint.h>   // int32_t, INT64_C, UINT8_MAX, etc.
#  include <math.h>     // float_t, double_t, etc.
#  include <float.h>    // FLT_EVAL_METHOD.

#  if !defined(FLT_EVAL_METHOD) && (defined(__FLT_EVAL_METHOD__) || defined(_FEVAL)) // GCC 3.x defines __FLT_EVAL_METHOD__ instead of the C99 standard FLT_EVAL_METHOD.
#    ifdef __FLT_EVAL_METHOD__
#      define FLT_EVAL_METHOD __FLT_EVAL_METHOD__
#    else
#      define FLT_EVAL_METHOD _FEVAL
#    endif
#  endif

    // MinGW GCC (up to at least v4.3.0-20080502) mistakenly neglects to define float_t and double_t.
    // This appears to be an acknowledged bug as of March 2008 and is scheduled to be fixed.
    // Similarly, Android uses a mix of custom standard library headers which don't define float_t and double_t.
#  if defined(__MINGW32__) || defined(EA_PLATFORM_ANDROID)
#    if defined(__FLT_EVAL_METHOD__)  
#      if(__FLT_EVAL_METHOD__== 0)
                typedef float float_t;
                typedef double double_t;
#      elif(__FLT_EVAL_METHOD__ == 1)
                typedef double float_t;
                typedef double double_t;
#      elif(__FLT_EVAL_METHOD__ == 2)
                typedef long double float_t;
                typedef long double double_t;
#      endif
#    else
            typedef float  float_t;
            typedef double double_t;
#    endif
#  endif 

    // Airplay's pretty broken for these types (at least as of 4.1)
#  if defined __S3E__

        typedef float float_t;
        typedef double double_t;

#    undef INT32_C
#    undef UINT32_C
#    undef INT64_C
#    undef UINT64_C
#    define  INT32_C(x)  x##L
#    define UINT32_C(x)  x##UL
#    define  INT64_C(x)  x##LL
#    define UINT64_C(x)  x##ULL

#    define EA_PRI_64_LENGTH_SPECIFIER "ll"
#    define EA_SCN_64_LENGTH_SPECIFIER "ll"

#    define SCNd16        "hd"
#    define SCNi16        "hi"
#    define SCNo16        "ho"
#    define SCNu16        "hu"
#    define SCNx16        "hx"

#    define SCNd32        "d" // This works for both 32 bit and 64 bit systems, as we assume LP64 conventions.
#    define SCNi32        "i"
#    define SCNo32        "o"
#    define SCNu32        "u"
#    define SCNx32        "x"

#    define SCNd64        EA_SCN_64_LENGTH_SPECIFIER "d"
#    define SCNi64        EA_SCN_64_LENGTH_SPECIFIER "i"
#    define SCNo64        EA_SCN_64_LENGTH_SPECIFIER "o"
#    define SCNu64        EA_SCN_64_LENGTH_SPECIFIER "u"
#    define SCNx64        EA_SCN_64_LENGTH_SPECIFIER "x"

#    define PRIdPTR       PRId32 // Usage of pointer values will generate warnings with 
#    define PRIiPTR       PRIi32 // some compilers because they are defined in terms of 
#    define PRIoPTR       PRIo32 // integers. However, you can't simply use "p" because
#    define PRIuPTR       PRIu32 // 'p' is interpreted in a specific and often different
#    define PRIxPTR       PRIx32 // way by the library.
#    define PRIXPTR       PRIX32

#    define PRId8     "hhd"
#    define PRIi8     "hhi"
#    define PRIo8     "hho"
#    define PRIu8     "hhu"
#    define PRIx8     "hhx"
#    define PRIX8     "hhX"

#    define PRId16        "hd"
#    define PRIi16        "hi"
#    define PRIo16        "ho"
#    define PRIu16        "hu"
#    define PRIx16        "hx"
#    define PRIX16        "hX"

#    define PRId32        "d" // This works for both 32 bit and 64 bit systems, as we assume LP64 conventions.
#    define PRIi32        "i"
#    define PRIo32        "o"
#    define PRIu32        "u"
#    define PRIx32        "x"
#    define PRIX32        "X"

#    define PRId64        EA_PRI_64_LENGTH_SPECIFIER "d"
#    define PRIi64        EA_PRI_64_LENGTH_SPECIFIER "i"
#    define PRIo64        EA_PRI_64_LENGTH_SPECIFIER "o"
#    define PRIu64        EA_PRI_64_LENGTH_SPECIFIER "u"
#    define PRIx64        EA_PRI_64_LENGTH_SPECIFIER "x"
#    define PRIX64        EA_PRI_64_LENGTH_SPECIFIER "X"
#  endif

    // The CodeSourcery definitions of PRIxPTR and SCNxPTR are broken for 32 bit systems.
#  if defined(__SIZEOF_SIZE_T__) && (__SIZEOF_SIZE_T__ == 4) && (defined(__have_long64) || defined(__have_longlong64) || defined(__S3E__))
#    undef  PRIdPTR
#    define PRIdPTR "d"
#    undef  PRIiPTR
#    define PRIiPTR "i"
#    undef  PRIoPTR
#    define PRIoPTR "o"
#    undef  PRIuPTR
#    define PRIuPTR "u"
#    undef  PRIxPTR
#    define PRIxPTR "x"
#    undef  PRIXPTR
#    define PRIXPTR "X"

#    undef  SCNdPTR
#    define SCNdPTR "d"
#    undef  SCNiPTR
#    define SCNiPTR "i"
#    undef  SCNoPTR
#    define SCNoPTR "o"
#    undef  SCNuPTR
#    define SCNuPTR "u"
#    undef  SCNxPTR
#    define SCNxPTR "x"
#  endif
#else // else we must implement types ourselves.

#  if !defined(__S3E__)
#    if !defined(__BIT_TYPES_DEFINED__) && !defined(__int8_t_defined)
            typedef signed char             int8_t;             //< 8 bit signed integer
#    endif
#    if !defined( __int8_t_defined )
            typedef signed short            int16_t;            //< 16 bit signed integer
            typedef signed int              int32_t;            //< 32 bit signed integer. This works for both 32 bit and 64 bit platforms, as we assume the LP64 is followed.
#      define __int8_t_defined
#    endif
            typedef unsigned char           uint8_t;            //< 8 bit unsigned integer
            typedef unsigned short         uint16_t;            //< 16 bit unsigned integer
#    if !defined( __uint32_t_defined )
            typedef unsigned int           uint32_t;            //< 32 bit unsigned integer. This works for both 32 bit and 64 bit platforms, as we assume the LP64 is followed.
#      define __uint32_t_defined
#    endif
#  endif

    // According to the C98/99 standard, FLT_EVAL_METHOD defines control the 
    // width used for floating point _t types.
#  if defined(__MWERKS__) && ((defined(_MSL_C99) && (_MSL_C99 == 1)) || (__MWERKS__ < 0x4000))
       // Metrowerks defines FLT_EVAL_METHOD and 
       // float_t/double_t under this condition.
#  elif defined(FLT_EVAL_METHOD)
#    if (FLT_EVAL_METHOD == 0)
            typedef float           float_t;
            typedef double          double_t;
#    elif (FLT_EVAL_METHOD == 1)
            typedef double          float_t;
            typedef double          double_t;
#    elif (FLT_EVAL_METHOD == 2)
            typedef long double     float_t;
            typedef long double     double_t;
#    endif
#  else
#    define FLT_EVAL_METHOD 0
        typedef float               float_t;
        typedef double              double_t;
#  endif

#  if defined(EA_PLATFORM_LINUX) || defined(EA_PLATFORM_PS3) || defined(EA_PLATFORM_PS3_SPU)
       typedef signed long long    int64_t;
       typedef unsigned long long  uint64_t;

#  elif defined(EA_PLATFORM_SUN) || defined(EA_PLATFORM_SGI)
#    if (EA_PLATFORM_PTR_SIZE == 4)
           typedef signed long long    int64_t;
           typedef unsigned long long  uint64_t;
#    else
           typedef signed long         int64_t;
           typedef unsigned long       uint64_t;
#    endif

#  elif defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_XBOX) || defined(EA_PLATFORM_XENON) || defined(EA_PLATFORM_MAC)
#    if defined(EA_COMPILER_MSVC) || defined(EA_COMPILER_BORLAND)  || defined(EA_COMPILER_INTEL)
           typedef signed __int64      int64_t;
           typedef unsigned __int64    uint64_t;
#    else // GCC, Metrowerks, etc.
           typedef long long           int64_t;
           typedef unsigned long long  uint64_t;
#    endif
#  elif defined(EA_PLATFORM_AIRPLAY)
#  else
       typedef signed long long    int64_t;
       typedef unsigned long long  uint64_t;
#  endif


    // ------------------------------------------------------------------------
    // macros for declaring constants in a portable way.
    //
    // e.g. int64_t  x =  INT64_C(1234567812345678);
    // e.g. int64_t  x =  INT64_C(0x1111111122222222);
    // e.g. uint64_t x = UINT64_C(0x1111111122222222);

#  ifndef INT8_C_DEFINED // If the user hasn't already defined these...
#    define INT8_C_DEFINED

        // VC++ 7.0 and earlier don't handle the LL suffix.
#    if defined(EA_COMPILER_MSVC) || defined(EA_COMPILER_BORLAND)
#      ifndef INT8_C
#        define   INT8_C(x)    int8_t(x)  // x##i8 doesn't work satisfactorilly because -128i8 generates an out of range warning.
#      endif
#      ifndef UINT8_C
#        define  UINT8_C(x)   uint8_t(x)
#      endif
#      ifndef INT16_C
#        define  INT16_C(x)   int16_t(x)  // x##i16 doesn't work satisfactorilly because -32768i8 generates an out of range warning.
#      endif
#      ifndef UINT16_C
#        define UINT16_C(x)  uint16_t(x)
#      endif
#      ifndef INT32_C
#        define  INT32_C(x)  x##i32
#      endif
#      ifndef UINT32_C
#        define UINT32_C(x)  x##ui32
#      endif
#      ifndef INT64_C
#        define  INT64_C(x)  x##i64
#      endif
#      ifndef UINT64_C
#        define UINT64_C(x)  x##ui64
#      endif

#    elif !defined(__STDC_CONSTANT_MACROS) // __STDC_CONSTANT_MACROS is defined by GCC 3 and later when INT8_C(), etc. are defined.
#      define   INT8_C(x)    int8_t(x)   // For the majority of compilers and platforms, long is 32 bits and long long is 64 bits.
#      define  UINT8_C(x)   uint8_t(x)
#      define  INT16_C(x)   int16_t(x)
#      define UINT16_C(x)  uint16_t(x)     // Possibly we should make this be uint16_t(x##u). Let's see how compilers react before changing this.
#      if defined(EA_PLATFORM_PS3)         // PS3 defines long as 64 bit, so we cannot use any size suffix.
#        define  INT32_C(x)  int32_t(x)
#        define UINT32_C(x)  uint32_t(x)
#      else                                // Else we are working on a platform whereby sizeof(long) == sizeof(int32_t).
#        define  INT32_C(x)  x##L
#        define UINT32_C(x)  x##UL
#      endif
#      define  INT64_C(x)  x##LL         // The way to deal with this is to compare ULONG_MAX to 0xffffffff and if not equal, then remove the L.
#      define UINT64_C(x)  x##ULL        // We need to follow a similar approach for LL.
#    endif
#  endif

    // ------------------------------------------------------------------------
    // type sizes
#  ifndef INT8_MAX_DEFINED // If the user hasn't already defined these...
#    define INT8_MAX_DEFINED

        // The value must be 2^(n-1)-1
#    ifndef INT8_MAX
#      define INT8_MAX                127
#    endif
#    ifndef INT16_MAX
#      define INT16_MAX               32767
#    endif
#    ifndef INT32_MAX
#      define INT32_MAX               2147483647
#    endif
#    ifndef INT64_MAX
#      define INT64_MAX               INT64_C(9223372036854775807)
#    endif

        // The value must be either -2^(n-1) or 1-2(n-1).
#    ifndef INT8_MIN
#      define INT8_MIN                -128
#    endif
#    ifndef INT16_MIN
#      define INT16_MIN               -32768
#    endif
#    ifndef INT32_MIN
#      define INT32_MIN               (-INT32_MAX - 1)  // -2147483648
#    endif
#    ifndef INT64_MIN
#      define INT64_MIN               (-INT64_MAX - 1)  // -9223372036854775808
#    endif

        // The value must be 2^n-1
#    ifndef UINT8_MAX
#      define UINT8_MAX               0xffU                        // 255
#    endif
#    ifndef UINT16_MAX
#      define UINT16_MAX              0xffffU                      // 65535
#    endif
#    ifndef UINT32_MAX
#      define UINT32_MAX              UINT32_C(0xffffffff)         // 4294967295
#    endif
#    ifndef UINT64_MAX
#      define UINT64_MAX              UINT64_C(0xffffffffffffffff) // 18446744073709551615 
#    endif
#  endif

    // ------------------------------------------------------------------------
    // sized printf and scanf format specifiers
    // See the C99 standard, section 7.8.1 -- Macros for format specifiers.
    //
    // The C99 standard specifies that inttypes.h only define printf/scanf 
    // format macros if __STDC_FORMAT_MACROS is defined before #including
    // inttypes.h. For consistency, we define both __STDC_FORMAT_MACROS and
    // the printf format specifiers here. We also skip the "least/most" 
    // variations of these specifiers, as we've decided to do so with 
    // basic types.
    //
    // For 64 bit systems, we assume the LP64 standard is followed 
    // (as opposed to ILP64, etc.) For 32 bit systems, we assume the 
    // ILP32 standard is followed. See:
    //    http://www.opengroup.org/public/tech/aspen/lp64_wp.htm 
    // for information about this. Thus, on both 32 and 64 bit platforms, 
    // %l refers to 32 bit data while %ll refers to 64 bit data. 

#  ifndef __STDC_FORMAT_MACROS
#    define __STDC_FORMAT_MACROS
#  endif

#  if defined(EA_COMPILER_MSVC) || defined(EA_COMPILER_BORLAND) // VC++ 7.1+ understands long long as a data type but doesn't accept %ll as a printf specifier.
#    define EA_PRI_64_LENGTH_SPECIFIER "I64"
#    define EA_SCN_64_LENGTH_SPECIFIER "I64"
#  else
#    define EA_PRI_64_LENGTH_SPECIFIER "ll"
#    define EA_SCN_64_LENGTH_SPECIFIER "ll"
#  endif // It turns out that some platforms use %q to represent a 64 bit value, but these are not relevant to us at this time.

    // Printf format specifiers
#  if defined(EA_COMPILER_IS_C99) || defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_METROWERKS) // || defined(EA_COMPILER_INTEL) ?
#    define PRId8     "hhd"
#    define PRIi8     "hhi"
#    define PRIo8     "hho"
#    define PRIu8     "hhu"
#    define PRIx8     "hhx"
#    define PRIX8     "hhX"
#  else // VC++, Borland, etc. which have no way to specify 8 bit values other than %c.
#    define PRId8     "c"  // This may not work properly but it at least will not crash. Try using 16 bit versions instead.
#    define PRIi8     "c"  //  "
#    define PRIo8     "o"  //  "
#    define PRIu8     "u"  //  "
#    define PRIx8     "x"  //  "
#    define PRIX8     "X"  //  "
#  endif

#  define PRId16        "hd"
#  define PRIi16        "hi"
#  define PRIo16        "ho"
#  define PRIu16        "hu"
#  define PRIx16        "hx"
#  define PRIX16        "hX"

#  define PRId32        "d" // This works for both 32 bit and 64 bit systems, as we assume LP64 conventions.
#  define PRIi32        "i"
#  define PRIo32        "o"
#  define PRIu32        "u"
#  define PRIx32        "x"
#  define PRIX32        "X"

#  define PRId64        EA_PRI_64_LENGTH_SPECIFIER "d"
#  define PRIi64        EA_PRI_64_LENGTH_SPECIFIER "i"
#  define PRIo64        EA_PRI_64_LENGTH_SPECIFIER "o"
#  define PRIu64        EA_PRI_64_LENGTH_SPECIFIER "u"
#  define PRIx64        EA_PRI_64_LENGTH_SPECIFIER "x"
#  define PRIX64        EA_PRI_64_LENGTH_SPECIFIER "X"

#  if (EA_PLATFORM_PTR_SIZE == 4)
#    define PRIdPTR       PRId32 // Usage of pointer values will generate warnings with 
#    define PRIiPTR       PRIi32 // some compilers because they are defined in terms of 
#    define PRIoPTR       PRIo32 // integers. However, you can't simply use "p" because
#    define PRIuPTR       PRIu32 // 'p' is interpreted in a specific and often different
#    define PRIxPTR       PRIx32 // way by the library.
#    define PRIXPTR       PRIX32
#  elif (EA_PLATFORM_PTR_SIZE == 8)
#    define PRIdPTR       PRId64
#    define PRIiPTR       PRIi64
#    define PRIoPTR       PRIo64
#    define PRIuPTR       PRIu64
#    define PRIxPTR       PRIx64
#    define PRIXPTR       PRIX64
#  endif

    // Scanf format specifiers
#  if defined(EA_COMPILER_IS_C99) || defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_METROWERKS) // || defined(EA_COMPILER_INTEL) ?
#    define SCNd8     "hhd"
#    define SCNi8     "hhi"
#    define SCNo8     "hho"
#    define SCNu8     "hhu"
#    define SCNx8     "hhx"
#  else // VC++, Borland, etc. which have no way to specify 8 bit values other than %c.
#    define SCNd8     "c" // This will not work properly but it at least will not crash. Try using 16 bit versions instead.
#    define SCNi8     "c" //  "
#    define SCNo8     "c" //  "
#    define SCNu8     "c" //  "
#    define SCNx8     "c" //  "
#  endif

#  define SCNd16        "hd"
#  define SCNi16        "hi"
#  define SCNo16        "ho"
#  define SCNu16        "hu"
#  define SCNx16        "hx"

#  define SCNd32        "d" // This works for both 32 bit and 64 bit systems, as we assume LP64 conventions.
#  define SCNi32        "i"
#  define SCNo32        "o"
#  define SCNu32        "u"
#  define SCNx32        "x"

#  define SCNd64        EA_SCN_64_LENGTH_SPECIFIER "d"
#  define SCNi64        EA_SCN_64_LENGTH_SPECIFIER "i"
#  define SCNo64        EA_SCN_64_LENGTH_SPECIFIER "o"
#  define SCNu64        EA_SCN_64_LENGTH_SPECIFIER "u"
#  define SCNx64        EA_SCN_64_LENGTH_SPECIFIER "x"

#  if (EA_PLATFORM_PTR_SIZE == 4)
#    define SCNdPTR       SCNd32 // Usage of pointer values will generate warnings with 
#    define SCNiPTR       SCNi32 // some compilers because they are defined in terms of 
#    define SCNoPTR       SCNo32 // integers. However, you can't simply use "p" because
#    define SCNuPTR       SCNu32 // 'p' is interpreted in a specific and often different
#    define SCNxPTR       SCNx32 // way by the library.
#  elif (EA_PLATFORM_PTR_SIZE == 8)
#    define SCNdPTR       SCNd64
#    define SCNiPTR       SCNi64
#    define SCNoPTR       SCNo64
#    define SCNuPTR       SCNu64
#    define SCNxPTR       SCNx64
#  endif

#endif


// ------------------------------------------------------------------------
// bool8_t
// The definition of a bool8_t is controversial with some, as it doesn't 
// act just like built-in bool. For example, you can assign -100 to it.
// 
#ifndef BOOL8_T_DEFINED // If the user hasn't already defined this...
#  define BOOL8_T_DEFINED
#  if defined(EA_COMPILER_MSVC) || defined(EA_COMPILER_METROWERKS) || (defined(EA_COMPILER_INTEL) && defined(EA_PLATFORM_WINDOWS)) || defined(EA_COMPILER_BORLAND)
#    if defined(__cplusplus)
            typedef bool bool8_t;
#    else
            typedef int8_t bool8_t;
#    endif
#  else // EA_COMPILER_GNUC generally uses 4 bytes per bool.
        typedef int8_t bool8_t;
#  endif
#endif


// ------------------------------------------------------------------------
// intptr_t / uintptr_t
// Integer type guaranteed to be big enough to hold
// a native pointer ( intptr_t is defined in STDDEF.H )
//
#if !defined(_INTPTR_T_DEFINED) && !defined(_intptr_t_defined) && !defined(EA_COMPILER_HAS_C99_TYPES)
#  if (EA_PLATFORM_PTR_SIZE == 4)
        typedef int32_t            intptr_t;
#  elif (EA_PLATFORM_PTR_SIZE == 8)
        typedef int64_t            intptr_t;
#  endif 

#  define _intptr_t_defined
#  define _INTPTR_T_DEFINED
#endif

#if !defined(_UINTPTR_T_DEFINED) && !defined(_uintptr_t_defined) && !defined(EA_COMPILER_HAS_C99_TYPES)
#  if (EA_PLATFORM_PTR_SIZE == 4)
        typedef uint32_t           uintptr_t;
#  elif (EA_PLATFORM_PTR_SIZE == 8)
        typedef uint64_t           uintptr_t;
#  endif 

#  define _uintptr_t_defined
#  define _UINTPTR_T_DEFINED
#endif

#if !defined(EA_COMPILER_HAS_INTTYPES)
#  ifndef INTMAX_T_DEFINED
#    define INTMAX_T_DEFINED

        // At this time, all supported compilers have int64_t as the max
        // integer type. Some compilers support a 128 bit inteter type,
        // but in those cases it is not a true int128_t but rather a 
        // crippled data type.
        typedef int64_t            intmax_t;
        typedef uint64_t           uintmax_t;
#  endif
#endif


// ------------------------------------------------------------------------
// ssize_t
// signed equivalent to size_t.
// This is defined by GCC but not by other compilers.
//
#if !defined(__GNUC__)
    // As of this writing, all non-GCC compilers significant to us implement 
    // uintptr_t the same as size_t. However, this isn't guaranteed to be 
    // so for all compilers, as size_t may be based on int, long, or long long.
#  if defined(_MSC_VER) && (EA_PLATFORM_PTR_SIZE == 8)
        typedef __int64 ssize_t;
#  elif !defined(__S3E__)
        typedef long ssize_t;
#  endif
#elif defined(EA_PLATFORM_UNIX) || defined(EA_PLATFORM_MINGW) || defined(__APPLE__) || defined(_BSD_SIZE_T_) // _BSD_SIZE_T_ indicates that Unix-like headers are present, even though it may not be a true Unix platform.
#  include <sys/types.h>
#endif


// ------------------------------------------------------------------------
// Character types

#if defined(EA_COMPILER_MSVC) || defined(EA_COMPILER_BORLAND)
#  if defined(EA_WCHAR_T_NON_NATIVE)
       // In this case, wchar_t is not defined unless we include 
       // wchar.h or if the compiler makes it built-in.
#    ifdef EA_COMPILER_MSVC
#      pragma warning(push, 3)
#    endif
#    include <wchar.h>
#    ifdef EA_COMPILER_MSVC
#      pragma warning(pop)
#    endif
#  endif
#endif


// ------------------------------------------------------------------------
// char8_t  -- Guaranteed to be equal to the compiler's char data type.
//             Some compilers implement char8_t as unsigned, though char 
//             is usually set to be signed.
//
// char16_t -- This is set to be an unsigned 16 bit value. If the compiler
//             has wchar_t as an unsigned 16 bit value, then char16_t is 
//             set to be the same thing as wchar_t in order to allow the 
//             user to use char16_t with standard wchar_t functions.
//
// char32_t -- This is set to be an unsigned 32 bit value. If the compiler
//             has wchar_t as an unsigned 32 bit value, then char32_t is 
//             set to be the same thing as wchar_t in order to allow the 
//             user to use char32_t with standard wchar_t functions.
//
// VS2010 unilaterally defines char16_t and char32_t in its yvals.h header
// unless _HAS_CHAR16_T_LANGUAGE_SUPPORT or _CHAR16T are defined. 
// However, VS2010 does not support the C++0x u"" and U"" string literals, 
// which makes its definition of char16_t and char32_t somewhat useless. 
// Until VC++ supports string literals, the buildystems should define 
// _CHAR16T and let EABase define char16_t and EA_CHAR16.
//
// GCC defines char16_t and char32_t in the C compiler in -std=gnu99 mode, 
// as __CHAR16_TYPE__ and __CHAR32_TYPE__, and for the C++ compiler 
// in -std=c++0x and -std=gnu++0x modes, as char16_t and char32_t too.

#if !defined(EA_CHAR16_NATIVE)
#  if defined(_MSC_VER) && (_MSC_VER >= 1600) && defined(_CHAR16T) || (defined(_HAS_CHAR16_T_LANGUAGE_SUPPORT) && _HAS_CHAR16_T_LANGUAGE_SUPPORT) // VS2010+
#    define EA_CHAR16_NATIVE 1
#  elif defined(__GNUC__) && ((__GNUC__ * 100 + __GNUC_MINOR__) >= 404) && (defined(__GXX_EXPERIMENTAL_CXX0X__) || defined(__STDC_VERSION__)) // g++ (C++ compiler) 4.4+ with -std=c++0x or gcc (C compiler) 4.4+ with -std=gnu99
#    define EA_CHAR16_NATIVE 1
#  else
#    define EA_CHAR16_NATIVE 0
#  endif
#endif

#if !defined(EA_CHAR32_NATIVE)
#  if defined(_MSC_VER) && (_MSC_VER >= 1600) && defined(_HAS_CHAR16_T_LANGUAGE_SUPPORT) && _HAS_CHAR16_T_LANGUAGE_SUPPORT // VS2010+
#    define EA_CHAR32_NATIVE 1
#  elif defined(__GNUC__) && ((__GNUC__ * 100 + __GNUC_MINOR__) >= 404) && (defined(__GXX_EXPERIMENTAL_CXX0X__) || defined(__STDC_VERSION__)) // g++ (C++ compiler) 4.4+ with -std=c++0x or gcc (C compiler) 4.4+ with -std=gnu99
#    define EA_CHAR32_NATIVE 1
#  else
#    define EA_CHAR32_NATIVE 0
#  endif
#endif


#ifndef CHAR8_T_DEFINED // If the user hasn't already defined these...
#  define CHAR8_T_DEFINED

#  if EA_CHAR16_NATIVE
        typedef char char8_t;

        // In C++, char16_t and char32_t are already defined by the compiler.
        // In MS C, char16_t and char32_t are already defined by the compiler/standard library.
        // In GCC C, __CHAR16_TYPE__ and __CHAR32_TYPE__ are defined instead, and we must define char16_t and char32_t from these.
#    if defined(__GNUC__) && !defined(__GXX_EXPERIMENTAL_CXX0X__) && defined(__CHAR16_TYPE__) // If using GCC and compiling in C...
            typedef __CHAR16_TYPE__ char16_t;
            typedef __CHAR32_TYPE__ char32_t;
#    endif
#  elif defined(EA_COMPILER_HAS_CHAR_16_32)
        typedef char char8_t;
#  elif (EA_WCHAR_SIZE == 2)
#    define _CHAR16T
        typedef char      char8_t;
        typedef wchar_t   char16_t;
        typedef uint32_t  char32_t;
#  else
        typedef char      char8_t;
        typedef uint16_t  char16_t;
        typedef wchar_t   char32_t;
#  endif
#endif


// EA_CHAR16 / EA_CHAR32
//
// Supports usage of portable string constants.
//
// Example usage:
//     const char16_t* str = EA_CHAR16("Hello world");
//     const char32_t* str = EA_CHAR32("Hello world");
//     const char16_t  c   = EA_CHAR16('\x3001');
//     const char32_t  c   = EA_CHAR32('\x3001');
//
#ifndef EA_CHAR16
#  if EA_CHAR16_NATIVE && !defined(_MSC_VER) // Microsoft doesn't support char16_t string literals.
#    define EA_CHAR16(s) u ## s
#  elif (EA_WCHAR_SIZE == 2)
#    define EA_CHAR16(s) L ## s
#  else
      //#define EA_CHAR16(s) // Impossible to implement.
#  endif
#endif

#ifndef EA_CHAR32
#  if EA_CHAR32_NATIVE && !defined(_MSC_VER) // Microsoft doesn't support char32_t string literals.
#    define EA_CHAR32(s) U ## s
#  elif (EA_WCHAR_SIZE == 2)
      //#define EA_CHAR32(s) // Impossible to implement.
#  else
#    define EA_CHAR32(s) L ## s
#  endif
#endif


// ------------------------------------------------------------------------
// EAArrayCount
//
// Returns the count of items in a built-in C array. This is a common technique
// which is often used to help properly calculate the number of items in an 
// array at runtime in order to prevent overruns, etc.
//
// Example usage:
//     int array[75];
//     size_t arrayCount = EAArrayCount(array); // arrayCount is 75.
//
#ifndef EAArrayCount
#  define EAArrayCount(x) (sizeof(x) / sizeof(x[0]))
#endif


// ------------------------------------------------------------------------
// static_assert
//
// C++0x static_assert (a.k.a. compile-time assert).
//
// Specification:
//     void static_assert(bool const_expression, const char* description);
//
// Example usage:
//     static_assert(sizeof(int) == 4, "int must be 32 bits");
//
#if !defined(EABASE_STATIC_ASSERT_ENABLED)
#  if defined(EA_DEBUG) || defined(_DEBUG)
#    define EABASE_STATIC_ASSERT_ENABLED 1
#  else
#    define EABASE_STATIC_ASSERT_ENABLED 0
#  endif
#endif

#ifndef EA_PREPROCESSOR_JOIN
#  define EA_PREPROCESSOR_JOIN(a, b)  EA_PREPROCESSOR_JOIN1(a, b)
#  define EA_PREPROCESSOR_JOIN1(a, b) EA_PREPROCESSOR_JOIN2(a, b)
#  define EA_PREPROCESSOR_JOIN2(a, b) a##b
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
    // static_assert is defined by the compiler for both C and C++.
#elif defined(__GNUC__) && defined(__GXX_EXPERIMENTAL_CXX0X__)
    // static_assert is defined by the compiler.
#elif defined(__clang__) && __has_feature(cxx_static_assert)
    // static_assert is defined by the compiler.
#else
#  if EABASE_STATIC_ASSERT_ENABLED
#    if defined(__COUNTER__) // If this VC++ extension is available...
#      define static_assert(expression, description) enum { EA_PREPROCESSOR_JOIN(static_assert_, __COUNTER__) = 1 / ((!!(expression)) ? 1 : 0) }
#    else
#      define static_assert(expression, description) enum { EA_PREPROCESSOR_JOIN(static_assert_, __LINE__) = 1 / ((!!(expression)) ? 1 : 0) }
#    endif
#  else
#    if defined(EA_COMPILER_METROWERKS)
#      if defined(__cplusplus)
#        define static_assert(expression, description) struct EA_PREPROCESSOR_JOIN(EACTAssertUnused_, __LINE__){ }
#      else
#        define static_assert(expression, description) enum { EA_PREPROCESSOR_JOIN(static_assert_, __LINE__) = 1 / ((!!(expression)) ? 1 : 0) }
#      endif
#    else
#      define static_assert(expression, description)
#    endif
#  endif
#endif


#endif // Header include guard









