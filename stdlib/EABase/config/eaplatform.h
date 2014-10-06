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
 * config/eaplatform.h
 *
 * Copyright (c) 2002 - 2005 Electronic Arts Inc. All rights reserved.
 * Maintained by Paul Pedriana, Maxis
 *
 *-----------------------------------------------------------------------------
 * Currently supported platform indentification defines include:
 *    EA_PLATFORM_PS3
 *    EA_PLATFORM_PS3_PPU
 *    EA_PLATFORM_PS3_SPU
 *    EA_PLATFORM_XENON (a.k.a. XBox2)
 *    EA_PLATFORM_MAC
 *    EA_PLATFORM_OSX
 *    EA_PLATFORM_LINUX
 *    EA_PLATFORM_WINDOWS
 *    EA_PLATFORM_WIN32
 *    EA_PLATFORM_WIN64
 *    EA_PLATFORM_HPUX
 *    EA_PLATFORM_SUN
 *    EA_PLATFORM_LRB (Larrabee)
 *    EA_PLATFORM_UNIX      (pseudo-platform; may be defined along with another platform like EA_PLATFORM_LINUX)
 *    EA_PLATFORM_CYGWIN    (pseudo-platform; may be defined along with another platform like EA_PLATFORM_LINUX)
 *    EA_PLATFORM_MINGW     (pseudo-platform; may be defined along with another platform like EA_PLATFORM_WINDOWS)
 *    EA_PLATFORM_MICROSOFT (pseudo-platform; may be defined along with another platform like EA_PLATFORM_WINDOWS)
 *    
 * Other definitions emanated from this file inclue:
 *    EA_PLATFORM_NAME = <string>
 *    EA_PLATFORM_DESCRIPTION = <string>
 *    EA_PROCESSOR_XXX
 *    EA_SYSTEM_LITTLE_ENDIAN | EA_SYSTEM_BIG_ENDIAN
 *    EA_ASM_STYLE_ATT | EA_ASM_STYLE_INTEL | EA_ASM_STYLE_MOTOROLA
 *    EA_PLATFORM_PTR_SIZE = <integer size in bytes>
 *    EA_PLATFORM_WORD_SIZE = <integer size in bytes>
 * 
 *---------------------------------------------------------------------------*/


#ifndef INCLUDED_eaplatform_H
#define INCLUDED_eaplatform_H


// Cygwin
// This is a pseudo-platform which will be defined along with EA_PLATFORM_LINUX when
// using the Cygwin build environment.
#if defined(__CYGWIN__)
    #define EA_PLATFORM_CYGWIN
    #define EA_PLATFORM_DESKTOP
#endif

// MinGW
// This is a pseudo-platform which will be defined along with EA_PLATFORM_WINDOWS when 
// using the MinGW Windows build environment.
#if defined(__MINGW32__) || defined(__MINGW64__)
    #define EA_PLATFORM_MINGW
    #define EA_PLATFORM_DESKTOP
#endif

// PlayStation 3 PPU (Primary Processing Unit)
#if defined(EA_PLATFORM_PS3_PPU) || defined(EA_PLATFORM_PS3) || defined(__PU__) || defined(__PPU__)
    #undef  EA_PLATFORM_PS3_PPU
    #define EA_PLATFORM_PS3_PPU 1
    #undef  EA_PLATFORM_PS3
    #define EA_PLATFORM_PS3 1
    #define EA_PLATFORM_NAME "PS3"
    #define EA_PROCESSOR_POWERPC
    #define EA_PROCESSOR_POWERPC_64
    #define EA_SYSTEM_BIG_ENDIAN
    #define EA_PLATFORM_DESCRIPTION "PS3 on PowerPC"
    #define EA_PLATFORM_CONSOLE

// PlayStation 3 SPU (Synergistic Processing Unit)
#elif defined(EA_PLATFORM_PS3_SPU) || defined(__SPU__)
    #undef  EA_PLATFORM_PS3_SPU
    #define EA_PLATFORM_PS3_SPU 1
    #define EA_PLATFORM_NAME "PS3 SPU"
    #define EA_PROCESSOR_SPU
    #define EA_SYSTEM_BIG_ENDIAN
    #define EA_PLATFORM_DESCRIPTION "PS3 SPU on SPU"
    #define EA_PLATFORM_CONSOLE

// XBox
// _XBOX is defined by the VC++ project, not the compiler. There is no way
// to tell if the compiler is compiling for XBox unless _XBOX is #defined 
// in the project files or otherwise. _M_IX86 is the VC++ way of detecting 
// an x86 target, which would mean XBox and not Xenon (a.k.a. XBox2).
#elif defined(EA_PLATFORM_XBOX) || (defined(_XBOX) && defined(_M_IX86))
    #undef  EA_PLATFORM_XBOX
    #define EA_PLATFORM_XBOX 1
    #define EA_PLATFORM_NAME "XBox"
    #define EA_PROCESSOR_X86
    #define EA_SYSTEM_LITTLE_ENDIAN
    #define EA_PLATFORM_DESCRIPTION "XBox on X86"
    #if defined(_MSC_VER) || defined(__ICL)
       #define EA_ASM_STYLE_INTEL
    #endif
    #define EA_PLATFORM_CONSOLE

// Xenon (XBox 360)
// The Xenon compiler doesn't define anything in particular to indicate that the 
// target is the Xenon platform. The Xenon SDK, however, expects that XBOX and
// _XBOX are #defined, so the project build file must make sure these are defined.
// Since the Xenon compiler in fact defines _M_PPC, we can use this information 
// to infer that Xenon is the target if neither _XENON nor _XBOX2 are specifically 
// defined by the project build file.
#elif defined(EA_PLATFORM_XENON) || defined(_XENON) || defined(_XBOX2) || ((defined(_XBOX) || defined(XBOX)) && defined(_M_PPC))
    #undef  EA_PLATFORM_XENON
    #define EA_PLATFORM_XENON 1
    #define EA_PLATFORM_NAME "Xenon"
    #define EA_PROCESSOR_POWERPC
    #define EA_PROCESSOR_POWERPC_64
    #define EA_SYSTEM_BIG_ENDIAN
    #define EA_PLATFORM_DESCRIPTION "Xenon on PowerPC"
    #if defined(_MSC_VER) || defined(__ICL)
       #define EA_ASM_STYLE_INTEL
    #endif
    #define EA_PLATFORM_CONSOLE
    #define EA_PLATFORM_MICROSOFT 1

// Larrabee                                           // This part to be removed once __LRB__ is supported by the Larrabee compiler in 2009.
#elif defined(EA_PLATFORM_LRB) || defined(__LRB__) || (defined(__EDG__) && defined(__ICC) && defined(__x86_64__))
    #undef  EA_PLATFORM_LRB
    #define EA_PLATFORM_LRB         1
    #define EA_PLATFORM_NAME        "Larrabee"
    #define EA_PLATFORM_DESCRIPTION "Larrabee on LRB1"
    #define EA_PROCESSOR_X86_64
    #if defined(BYTE_ORDER) && (BYTE_ORDER == 4321)
        #define EA_SYSTEM_BIG_ENDIAN
    #else
        #define EA_SYSTEM_LITTLE_ENDIAN
    #endif
    #define EA_PROCESSOR_LRB
    #define EA_PROCESSOR_LRB1       // Larrabee version 1
    #define EA_ASM_STYLE_ATT        // Both types of asm style 
    #define EA_ASM_STYLE_INTEL      // are supported.
    #define EA_PLATFORM_DESKTOP

// Android (Google phone OS)
#elif defined(EA_PLATFORM_ANDROID) || defined(__ANDROID__)
    #undef  EA_PLATFORM_ANDROID
    #define EA_PLATFORM_ANDROID 1
    #define EA_PLATFORM_LINUX 1
    #define EA_PLATFORM_UNIX 1
    #define EA_PLATFORM_NAME "Android"
    #define EA_ASM_STYLE_ATT
    #if defined(__arm__)
        #define EA_PROCESSOR_ARM
        #define EA_PLATFORM_DESCRIPTION "Android on ARM"
    #else
        #error Unknown processor
    #endif
    #if !defined(EA_SYSTEM_BIG_ENDIAN) && !defined(EA_SYSTEM_LITTLE_ENDIAN)
        #define EA_SYSTEM_LITTLE_ENDIAN
    #endif
    #define EA_PLATFORM_MOBILE

// Palm OS for Mobile (Linux variant)
#elif defined(EA_PLATFORM_PALM)
    #undef  EA_PLATFORM_PALM
    #define EA_PLATFORM_PALM 1
    #define EA_PLATFORM_LINUX 1
    #define EA_PLATFORM_UNIX 1
    #define EA_PLATFORM_NAME "Palm"
    #define EA_POSIX_THREADS_AVAILABLE 1
    #define EA_ASM_STYLE_ATT
    #if defined(__arm__)
        #define EA_PROCESSOR_ARM
        #define EA_PLATFORM_DESCRIPTION "Palm on ARM"
    #else
        #error Unknown processor
    #endif
    #if !defined(EA_SYSTEM_BIG_ENDIAN) && !defined(EA_SYSTEM_LITTLE_ENDIAN)
        #define EA_SYSTEM_LITTLE_ENDIAN
    #endif
    #define EA_PLATFORM_MOBILE

// Airplay
#elif defined(EA_PLATFORM_AIRPLAY) || defined(__S3E__)
    #undef  EA_PLATFORM_AIRPLAY
    #define EA_PLATFORM_AIRPLAY
    #define EA_PLATFORM_NAME "Airplay"
    #if defined(__arm__)
        #define EA_PROCESSOR_ARM
        #define EA_PLATFORM_DESCRIPTION "Airplay on ARM"
        #define EA_ASM_STYLE_ATT
        #undef _MSC_VER    
    #elif defined(_M_IX86)
        #define EA_PROCESSOR_X86
        #define EA_PLATFORM_DESCRIPTION "Airplay on x86"
        #define EA_ASM_STYLE_INTEL
    #else
        #error Unknown processor
    #endif
    #if !defined(EA_SYSTEM_BIG_ENDIAN) && !defined(EA_SYSTEM_LITTLE_ENDIAN)
        #if defined(HAVE_BIG_ENDIAN) || (defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && (__BYTE_ORDER == __BIG_ENDIAN)))
            #define EA_SYSTEM_BIG_ENDIAN
        #else
            #define EA_SYSTEM_LITTLE_ENDIAN
        #endif
    #endif
    #define EA_PLATFORM_MOBILE

// Samsung Bada OS for Mobile (Linux variant)
#elif defined(EA_PLATFORM_BADA)
    #undef  EA_PLATFORM_BADA
    #define EA_PLATFORM_BADA 1
  //#define EA_PLATFORM_LINUX 1     // The underlying OS is Linux, but the app mostly doesn't see this.
  //#define EA_PLATFORM_UNIX 1
    #define EA_PLATFORM_NAME "bada"
    #define EA_ASM_STYLE_ATT
    #if defined(__arm__)
        #define EA_PROCESSOR_ARM
        #define EA_PLATFORM_DESCRIPTION "bada on ARM"
    #elif defined(__i386__)
        #define EA_PLATFORM_BADA_SIMULATOR
        #define EA_PROCESSOR_X86
        #define EA_PLATFORM_DESCRIPTION "bada simulator on x86"
    #else
        #error Unknown processor
    #endif
    #if !defined(EA_SYSTEM_BIG_ENDIAN) && !defined(EA_SYSTEM_LITTLE_ENDIAN)
        #define EA_SYSTEM_LITTLE_ENDIAN
    #endif
    #define EA_PLATFORM_MOBILE

#elif defined(__APPLE__) && __APPLE__ 
    #include <TargetConditionals.h>

    // Apple family of operating systems.
    #define EA_PLATFORM_APPLE
    
    // iPhone 
    // TARGET_OS_IPHONE will be undefined on an unknown compiler, and will be defined on gcc.
    #if defined(EA_PLATFORM_IPHONE) || defined(__IPHONE__) || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE) || (defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR)
        #undef  EA_PLATFORM_IPHONE
        #define EA_PLATFORM_IPHONE 1
        #define EA_PLATFORM_NAME "iPhone"
        #define EA_ASM_STYLE_ATT
        #define EA_POSIX_THREADS_AVAILABLE 1
        #if defined(__arm__)
            #define EA_PROCESSOR_ARM
            #define EA_SYSTEM_LITTLE_ENDIAN
            #define EA_PLATFORM_DESCRIPTION "iPhone on ARM"
        #elif defined(__i386__)
            #define EA_PLATFORM_IPHONE_SIMULATOR
            #define EA_PROCESSOR_X86
            #define EA_SYSTEM_LITTLE_ENDIAN
            #define EA_PLATFORM_DESCRIPTION "iPhone simulator on x86"
        #else
            #error Unknown processor
        #endif
        #define EA_PLATFORM_MOBILE

    // Macintosh OSX
    // TARGET_OS_MAC is defined by the Metrowerks and older AppleC compilers.
    // Howerver, TARGET_OS_MAC is defined to be 1 in all cases.
    // __i386__ and __intel__ are defined by the GCC compiler.
    // __dest_os is defined by the Metrowerks compiler.
    // __MACH__ is defined by the Metrowerks and GCC compilers.
    // powerc and __powerc are defined by the Metrowerks and GCC compilers.
    #elif defined(EA_PLATFORM_OSX) || defined(__MACH__) || (defined(__MSL__) && (__dest_os == __mac_os_x))
        #undef  EA_PLATFORM_OSX
        #define EA_PLATFORM_OSX 1
        #define EA_PLATFORM_UNIX 1
        #define EA_PLATFORM_NAME "OSX"
        #if defined(__i386__) || defined(__intel__)
            #define EA_PROCESSOR_X86
            #define EA_SYSTEM_LITTLE_ENDIAN
            #define EA_PLATFORM_DESCRIPTION "OSX on x86"
        #elif defined(__x86_64) || defined(__amd64)
            #define EA_PROCESSOR_X86_64
            #define EA_SYSTEM_LITTLE_ENDIAN
            #define EA_PLATFORM_DESCRIPTION "OSX on x86-64"
        #elif defined(__arm__)
            #define EA_PROCESSOR_ARM
            #define EA_SYSTEM_LITTLE_ENDIAN
            #define EA_PLATFORM_DESCRIPTION "OSX on ARM"
        #elif defined(__POWERPC64__) || defined(__powerpc64__)
            #define EA_PROCESSOR_POWERPC
            #define EA_PROCESSOR_POWERPC_64
            #define EA_SYSTEM_BIG_ENDIAN
            #define EA_PLATFORM_DESCRIPTION "OSX on PowerPC 64"
        #elif defined(__POWERPC__) || defined(__powerpc__)
            #define EA_PROCESSOR_POWERPC
            #define EA_PROCESSOR_POWERPC_32
            #define EA_SYSTEM_BIG_ENDIAN
            #define EA_PLATFORM_DESCRIPTION "OSX on PowerPC"
        #else
            #error Unknown processor
        #endif
        #if defined(__GNUC__)
            #define EA_ASM_STYLE_ATT
        #else
            #define EA_ASM_STYLE_MOTOROLA
        #endif
        #define EA_PLATFORM_DESKTOP

    #else
        #error Unknown Apple Platform
    #endif

// Linux
// __linux and __linux__ are defined by the GCC and Borland compiler.
// __i386__ and __intel__ are defined by the GCC compiler.
// __i386__ is defined by the Metrowerks compiler.
// _M_IX86 is defined by the Borland compiler.
// __sparc__ is defined by the GCC compiler.
// __powerpc__ is defined by the GCC compiler.
#elif defined(EA_PLATFORM_LINUX) || (defined(__linux) || defined(__linux__))
    #undef  EA_PLATFORM_LINUX
    #define EA_PLATFORM_LINUX 1
    #define EA_PLATFORM_UNIX 1
    #define EA_PLATFORM_NAME "Linux"
    #if defined(__i386__) || defined(__intel__) || defined(_M_IX86)
        #define EA_PROCESSOR_X86
        #define EA_SYSTEM_LITTLE_ENDIAN
        #define EA_PLATFORM_DESCRIPTION "Linux on x86"
    #elif defined(__x86_64__)
        #define EA_PROCESSOR_X86_64
        #define EA_SYSTEM_LITTLE_ENDIAN
        #define EA_PLATFORM_DESCRIPTION "Linux on x86-64"
    #elif defined(__powerpc64__)
        #define EA_PROCESSOR_POWERPC
        #define EA_PROCESSOR_POWERPC_64
        #define EA_SYSTEM_BIG_ENDIAN
        #define EA_PLATFORM_DESCRIPTION "Linux on PowerPC 64"
    #elif defined(__powerpc__)
        #define EA_PROCESSOR_POWERPC
        #define EA_PROCESSOR_POWERPC_32
        #define EA_SYSTEM_BIG_ENDIAN
        #define EA_PLATFORM_DESCRIPTION "Linux on PowerPC"
    #elif defined(__arm__)
        #define EA_PROCESSOR_ARM
        #define EA_SYSTEM_LITTLE_ENDIAN
        #define EA_PLATFORM_DESCRIPTION "Linux on ARM"
    #else
        #error Unknown processor
        #error Unknown endianness
    #endif
    #if defined(__GNUC__)
        #define EA_ASM_STYLE_ATT
    #endif
    #define EA_PLATFORM_DESKTOP

// Win CE (Windows mobile)
#elif defined(EA_PLATFORM_WINCE) || defined(_WIN32_WCE)
    #undef  EA_PLATFORM_WINCE
    #define EA_PLATFORM_WINCE 1
    #define EA_PLATFORM_NAME "WinCE"
    #define EA_ASM_STYLE_INTEL
    #define EA_SYSTEM_LITTLE_ENDIAN
    #if defined(_M_ARM) // Also there is _M_ARMT
        #define EA_PROCESSOR_ARM
        #define EA_PLATFORM_DESCRIPTION "Windows CE on ARM"
    #elif defined(_M_IX86)
        #define EA_PROCESSOR_X86
        #define EA_PLATFORM_DESCRIPTION "Windows CE on X86"
    #else //Possibly other Windows CE variants
        #error Unknown processor
    #endif
    #define EA_PLATFORM_MOBILE

// Windows
// _WIN32 is defined by the VC++, Intel and GCC compilers.
// _WIN64 is defined by the VC++, Intel and GCC compilers.
// __WIN32__ is defined by the Borland compiler.
// __INTEL__ is defined by the Metrowerks compiler.
// _M_IX86, _M_AMD64 and _M_IA64 are defined by the VC++, Intel, and Borland compilers.
// _X86_, _AMD64_, and _IA64_ are defined by the Metrowerks compiler.
// _M_ARM is defined by the VC++ compiler.
#elif (defined(EA_PLATFORM_WINDOWS) || (defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || (defined(__MWERKS__) && defined(_X86_)))) && !defined(_XBOX)
    #undef  EA_PLATFORM_WINDOWS
    #define EA_PLATFORM_WINDOWS 1
    #define EA_PLATFORM_NAME "Windows"
    #ifdef _WIN64 // VC++ defines both _WIN32 and _WIN64 when compiling for Win64.
        #define EA_PLATFORM_WIN64
    #else
        #define EA_PLATFORM_WIN32
    #endif
    #if defined(_M_AMD64) || defined(_AMD64_) || defined(__x86_64__)
        #define EA_PROCESSOR_X86_64
        #define EA_SYSTEM_LITTLE_ENDIAN
        #define EA_PLATFORM_DESCRIPTION "Windows on X86-64"
    #elif defined(_M_IX86) || defined(_X86_)
        #define EA_PROCESSOR_X86
        #define EA_SYSTEM_LITTLE_ENDIAN
        #define EA_PLATFORM_DESCRIPTION "Windows on X86"
    #elif defined(_M_IA64) || defined(_IA64_)
        #define EA_PROCESSOR_IA64
        #define EA_SYSTEM_LITTLE_ENDIAN
        #define EA_PLATFORM_DESCRIPTION "Windows on IA-64"
    #elif defined(_M_ARM)
        #define EA_PROCESSOR_ARM
        #define EA_SYSTEM_LITTLE_ENDIAN
        #define EA_PLATFORM_DESCRIPTION "Windows CE on ARM"
    #else //Possibly other Windows CE variants
        #error Unknown processor
        #error Unknown endianness
    #endif
    #if defined(__GNUC__)
        #define EA_ASM_STYLE_ATT
    #elif defined(_MSC_VER) || defined(__BORLANDC__) || defined(__ICL)
        #define EA_ASM_STYLE_INTEL
    #endif
    #define EA_PLATFORM_DESKTOP
    #define EA_PLATFORM_MICROSOFT 1

// Sun (Solaris)
// __SUNPRO_CC is defined by the Sun compiler.
// __sun is defined by the GCC compiler.
// __i386 is defined by the Sun and GCC compilers.
// __sparc is defined by the Sun and GCC compilers.
#elif defined(EA_PLATFORM_SUN) || (defined(__SUNPRO_CC) || defined(__sun))
    #undef  EA_PLATFORM_SUN
    #define EA_PLATFORM_SUN 1
    #define EA_PLATFORM_UNIX 1
    #define EA_PLATFORM_NAME "SUN"
    #if defined(__i386)
        #define EA_PROCESSOR_X86
        #define EA_SYSTEM_LITTLE_ENDIAN
        #define EA_PLATFORM_DESCRIPTION "SUN on x86"
    #elif defined(__sparc)
        #define EA_PROCESSOR_SPARC
        #define EA_SYSTEM_BIG_ENDIAN
        #define EA_PLATFORM_DESCRIPTION "SUN on Sparc"
    #else
        #error Unknown processor
        #error Unknown endianness
    #endif
    #define EA_PLATFORM_DESKTOP

#else
	#define EA_PROCESSOR_X86
	#define EA_SYSTEM_LITTLE_ENDIAN
	#define EA_PLATFORM_DESCRIPTION "IncludeOS on x86"
    //#error Unknown platform
    //#error Unknown processor
    //#error Unknown endianness
#endif



// EA_PLATFORM_PTR_SIZE
// Platform pointer size; same as sizeof(void*).
// This is not the same as sizeof(int), as int is usually 32 bits on 
// even 64 bit platforms. 
//
// _WIN64 is defined by Win64 compilers, such as VC++.
// _M_IA64 is defined by VC++ and Intel compilers for IA64 processors.
// __LP64__ is defined by HP compilers for the LP64 standard.
// _LP64 is defined by the GCC and Sun compilers for the LP64 standard.
// __ia64__ is defined by the GCC compiler for IA64 processors.
// __arch64__ is defined by the Sparc compiler for 64 bit processors.
// __mips64__ is defined by the GCC compiler for MIPS processors.
// __powerpc64__ is defined by the GCC compiler for PowerPC processors.
// __64BIT__ is defined by the AIX compiler for 64 bit processors.
// __sizeof_ptr is defined by the ARM compiler (armcc, armcpp).
//
#ifndef EA_PLATFORM_PTR_SIZE
    #if defined(__WORDSIZE) // Defined by some variations of GCC.
        #define EA_PLATFORM_PTR_SIZE ((__WORDSIZE) / 8)
    #elif defined(_WIN64) || defined(__LP64__) || defined(_LP64) || defined(_M_IA64) || defined(__ia64__) || defined(__arch64__) || defined(__mips64__) || defined(__64BIT__) 
        #define EA_PLATFORM_PTR_SIZE 8
    #elif defined(__CC_ARM) && (__sizeof_ptr == 8)
        #define EA_PLATFORM_PTR_SIZE 8
    #else
        #define EA_PLATFORM_PTR_SIZE 4
    #endif
#endif



// EA_PLATFORM_WORD_SIZE
// This defines the size of a machine word. This will be the same as 
// the size of registers on the machine but not necessarily the same
// as the size of pointers on the machine. A number of 64 bit platforms
// have 64 bit registers but 32 bit pointers.
//
#ifndef EA_PLATFORM_WORD_SIZE
   #if defined(EA_PLATFORM_XENON) || defined(EA_PLATFORM_PS3) 
      #define EA_PLATFORM_WORD_SIZE 8
   #else
      #define EA_PLATFORM_WORD_SIZE EA_PLATFORM_PTR_SIZE
   #endif
#endif



// Disabled until and unless deemed useful:
//
// Platform integer types
// These definitions allow us to define other things properly, such as 
// sized integer types. In order to bring some order to this chaos, 
// we follow a variation of the standard LP64 conventions defined at:
//    http://www.opengroup.org/public/tech/aspen/lp64_wp.htm 
//
// #if defined(EA_PLATFORM_LINUX) || defined(EA_PLATFORM_OSX) || defined(EA_PLATFORM_XBOX) || defined(EA_PLATFORM_XENON)
//    #define EA_PLATFORM_ILP32_LL64         // int, long, ptr = 32 bits; long long = 64 bits.
// 
// #elif defined(EA_PLATFORM_SUN) || defined(EA_PLATFORM_SGI)
//    #if (EA_PLATFORM_WORD_SIZE == 32)
//       #define ILP32_LL64                  // int, long, ptr = 32 bits; long long = 64 bits.
//    #else // 64 bit platform
//       #define EA_PLATFORM_I32_LLLP64      // int = 32 bits; long, long long, ptr = 64 bits.
//    #endif
// 
// #elif defined(EA_PLATFORM_WINDOWS)
//    #if (EA_PLATFORM_WORD_SIZE == 32)
//       #define ILP32_LL64                  // int, long, ptr = 32 bits; long long = 64 bits.
//    #else // 64 bit platform
//       #if defined(__MWERKS__) || defined(__GNUC__)
//          #define EA_PLATFORM_I32_LLLP64   // int = 32 bits; long, long long, ptr = 64 bits.
//       #else // MSVC
//          #define EA_PLATFORM_IL32_LLP64   // int, long = 32 bits; long long, ptr = 64 bits.
//       #endif
//    #endif
// #endif


#endif // INCLUDED_eaplatform_H














