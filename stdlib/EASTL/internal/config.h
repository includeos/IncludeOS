/*
Copyright (C) 2005,2009-2010 Electronic Arts, Inc.  All rights reserved.

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

///////////////////////////////////////////////////////////////////////////////
// EASTL/internal/config.h
// Written and maintained by Paul Pedriana - 2005.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_INTERNAL_CONFIG_H
#define EASTL_INTERNAL_CONFIG_H


///////////////////////////////////////////////////////////////////////////////
// ReadMe
//
// This is the EASTL configuration file. All configurable parameters of EASTL
// are controlled through this file. However, all the settings here can be 
// manually overridden by the user. There are three ways for a user to override
// the settings in this file:
//
//     - Simply edit this file.
//     - Define EASTL_USER_CONFIG_HEADER.
//     - Predefine individual defines (e.g. EASTL_ASSERT).
//
///////////////////////////////////////////////////////////////////////////////




///////////////////////////////////////////////////////////////////////////////
// EASTL_USER_CONFIG_HEADER
//
// This allows the user to define a header file to be #included before the 
// EASTL config.h contents are compiled. A primary use of this is to override
// the contents of this config.h file. Note that all the settings below in 
// this file are user-overridable.
// 
// Example usage:
//     #define EASTL_USER_CONFIG_HEADER "MyConfigOverrides.h"
//     #include <EASTL/vector.h>
//
///////////////////////////////////////////////////////////////////////////////

#ifdef EASTL_USER_CONFIG_HEADER
    #include EASTL_USER_CONFIG_HEADER
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_EABASE_DISABLED
//
// The user can disable EABase usage and manually supply the configuration
// via defining EASTL_EABASE_DISABLED and defining the appropriate entities
// globally or via the above EASTL_USER_CONFIG_HEADER.
// 
// Example usage:
//     #define EASTL_EABASE_DISABLED
//     #include <EASTL/vector.h>
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_EABASE_DISABLED
    #include <EABase/eabase.h>
#endif



///////////////////////////////////////////////////////////////////////////////
// VC++ bug fix.
///////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
    // VC8 has a bug whereby it generates a warning when malloc.h is #included
    // by its headers instead of by yours. There is no practical solution but
    // to pre-empt the #include of malloc.h with our own inclusion of it. 
    // The only other alternative is to disable the warning globally, which is
    // something we try to avoid as much as possible.
    #pragma warning(push, 0)
    #include <malloc.h>
    #pragma warning(pop)
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_VERSION
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
//     printf("EASTL version: %s", EASTL_VERSION);
//     printf("EASTL version: %d.%d.%d", EASTL_VERSION_N / 10000 % 100, EASTL_VERSION_N / 100 % 100, EASTL_VERSION_N % 100);
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_VERSION
    #define EASTL_VERSION   "1.11.03"
    #define EASTL_VERSION_N  11103
#endif



///////////////////////////////////////////////////////////////////////////////
// EA_PLATFORM_MICROSOFT
//
// Defined as 1 or undefined.
// Implements support for the definition of EA_PLATFORM_MICROSOFT for the case
// of using EABase versions prior to the addition of its EA_PLATFORM_MICROSOFT support.
//
#if (EABASE_VERSION_N < 20022) && !defined(EA_PLATFORM_MICROSOFT)
    #if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_XENON)
        #define EA_PLATFORM_MICROSOFT 1
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EA_COMPILER_NO_STANDARD_CPP_LIBRARY
//
// Defined as 1 or undefined.
// Implements support for the definition of EA_COMPILER_NO_STANDARD_CPP_LIBRARY for the case
// of using EABase versions prior to the addition of its EA_COMPILER_NO_STANDARD_CPP_LIBRARY support.
//
#if (EABASE_VERSION_N < 20022) && !defined(EA_COMPILER_NO_STANDARD_CPP_LIBRARY)
    #if defined(EA_PLATFORM_ANDROID)
        #define EA_COMPILER_NO_STANDARD_CPP_LIBRARY 1
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EA_COMPILER_NO_RTTI
//
// Defined as 1 or undefined.
// Implements support for the definition of EA_COMPILER_NO_RTTI for the case
// of using EABase versions prior to the addition of its EA_COMPILER_NO_RTTI support.
//
#if (EABASE_VERSION_N < 20022) && !defined(EA_COMPILER_NO_RTTI)
    #if defined(__SNC__) && !defined(__RTTI)
        #define EA_COMPILER_NO_RTTI
    #elif defined(__GXX_ABI_VERSION) && !defined(__GXX_RTTI)
        #define EA_COMPILER_NO_RTTI
    #elif defined(_MSC_VER) && !defined(_CPPRTTI)
        #define EA_COMPILER_NO_RTTI
    #elif defined(__MWERKS__)
        #if !__option(RTTI)
            #define EA_COMPILER_NO_RTTI
        #endif
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL namespace
//
// We define this so that users that #include this config file can reference 
// these namespaces without seeing any other files that happen to use them.
///////////////////////////////////////////////////////////////////////////////

/// EA Standard Template Library
namespace eastl
{
    // Intentionally empty. 
}




///////////////////////////////////////////////////////////////////////////////
// EASTL_DEBUG
//
// Defined as an integer >= 0. Default is 1 for debug builds and 0 for 
// release builds. This define is also a master switch for the default value 
// of some other settings.
//
// Example usage:
//    #if EASTL_DEBUG
//       ...
//    #endif
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_DEBUG
    #if defined(EA_DEBUG) || defined(_DEBUG)
        #define EASTL_DEBUG 1
    #else
        #define EASTL_DEBUG 0
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_DEBUGPARAMS_LEVEL
//
// EASTL_DEBUGPARAMS_LEVEL controls what debug information is passed through to
// the allocator by default.
// This value may be defined by the user ... if not it will default to 1 for
// EA_DEBUG builds, otherwise 0.
//
//  0 - no debug information is passed through to allocator calls.
//  1 - 'name' is passed through to allocator calls.
//  2 - 'name', __FILE__, and __LINE__ are passed through to allocator calls.
//
// This parameter mirrors the equivalent parameter in the CoreAllocator package.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_DEBUGPARAMS_LEVEL
    #if EASTL_DEBUG
        #define EASTL_DEBUGPARAMS_LEVEL 2
    #else
        #define EASTL_DEBUGPARAMS_LEVEL 0
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_DLL
//
// Defined as 0 or 1. The default is dependent on the definition of EA_DLL.
// If EA_DLL is defined, then EASTL_DLL is 1, else EASTL_DLL is 0.
// EA_DLL is a define that controls DLL builds within the EAConfig build system. 
// EASTL_DLL controls whether EASTL is built and used as a DLL. 
// Normally you wouldn't do such a thing, but there are use cases for such
// a thing, particularly in the case of embedding C++ into C# applications.
//
#ifndef EASTL_DLL
    #if defined(EA_DLL)
        #define EASTL_DLL 1
    #else
        #define EASTL_DLL 0
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_API
//
// This is used to label functions as DLL exports under Microsoft platforms.
// If EA_DLL is defined, then the user is building EASTL as a DLL and EASTL's
// non-templated functions will be exported. EASTL template functions are not
// labelled as EASTL_API (and are thus not exported in a DLL build). This is 
// because it's not possible (or at least unsafe) to implement inline templated 
// functions in a DLL.
//
// Example usage of EASTL_API:
//    EASTL_API int someVariable = 10;  // Export someVariable in a DLL build.
//
//    struct EASTL_API SomeClass{       // Export SomeClass and its member functions in a DLL build.
//    };
//
//    EASTL_API void SomeFunction();    // Export SomeFunction in a DLL build.
//
//
#if defined(EA_DLL) && !defined(EASTL_DLL)
    #define EASTL_DLL 1
#endif

#ifndef EASTL_API // If the build file hasn't already defined this to be dllexport...
    #if EASTL_DLL && defined(_MSC_VER)
        #define EASTL_API           __declspec(dllimport)
        #define EASTL_TEMPLATE_API  // Not sure if there is anything we can do here.
    #else
        #define EASTL_API
        #define EASTL_TEMPLATE_API
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EASTL_NAME_ENABLED / EASTL_NAME / EASTL_NAME_VAL
//
// Used to wrap debug string names. In a release build, the definition 
// goes away. These are present to avoid release build compiler warnings 
// and to make code simpler.
//
// Example usage of EASTL_NAME:
//    // pName will defined away in a release build and thus prevent compiler warnings.
//    void allocator::set_name(const char* EASTL_NAME(pName))
//    {
//        #if EASTL_NAME_ENABLED
//            mpName = pName;
//        #endif
//    }
//
// Example usage of EASTL_NAME_VAL:
//    // "xxx" is defined to NULL in a release build.
//    vector<T, Allocator>::vector(const allocator_type& allocator = allocator_type(EASTL_NAME_VAL("xxx")));
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_NAME_ENABLED
    #define EASTL_NAME_ENABLED EASTL_DEBUG
#endif

#ifndef EASTL_NAME
    #if EASTL_NAME_ENABLED
        #define EASTL_NAME(x)      x
        #define EASTL_NAME_VAL(x)  x
    #else
        #define EASTL_NAME(x)
        #define EASTL_NAME_VAL(x) ((const char*)NULL)
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_DEFAULT_NAME_PREFIX
//
// Defined as a string literal. Defaults to "EASTL".
// This define is used as the default name for EASTL where such a thing is
// referenced in EASTL. For example, if the user doesn't specify an allocator
// name for their deque, it is named "EASTL deque". However, you can override
// this to say "SuperBaseball deque" by changing EASTL_DEFAULT_NAME_PREFIX.
//
// Example usage (which is simply taken from how deque.h uses this define):
//     #ifndef EASTL_DEQUE_DEFAULT_NAME
//         #define EASTL_DEQUE_DEFAULT_NAME   EASTL_DEFAULT_NAME_PREFIX " deque"
//     #endif
//
#ifndef EASTL_DEFAULT_NAME_PREFIX
    #define EASTL_DEFAULT_NAME_PREFIX "EASTL"
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_ASSERT_ENABLED
//
// Defined as 0 or non-zero. Default is same as EASTL_DEBUG.
// If EASTL_ASSERT_ENABLED is non-zero, then asserts will be executed via 
// the assertion mechanism.
//
// Example usage:
//     #if EASTL_ASSERT_ENABLED
//         EASTL_ASSERT(v.size() > 17);
//     #endif
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_ASSERT_ENABLED
    #define EASTL_ASSERT_ENABLED EASTL_DEBUG
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
//
// Defined as 0 or non-zero. Default is same as EASTL_ASSERT_ENABLED.
// This is like EASTL_ASSERT_ENABLED, except it is for empty container 
// references. Sometime people like to be able to take a reference to 
// the front of the container, but not use it if the container is empty.
// In practice it's often easier and more efficient to do this than to write 
// extra code to check if the container is empty. 
//
// Example usage:
//     template <typename T, typename Allocator>
//     inline typename vector<T, Allocator>::reference
//     vector<T, Allocator>::front()
//     {
//         #if EASTL_ASSERT_ENABLED
//             EASTL_ASSERT(mpEnd > mpBegin);
//         #endif
// 
//         return *mpBegin;
//     }
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
    #define EASTL_EMPTY_REFERENCE_ASSERT_ENABLED EASTL_ASSERT_ENABLED
#endif



///////////////////////////////////////////////////////////////////////////////
// SetAssertionFailureFunction
//
// Allows the user to set a custom assertion failure mechanism.
//
// Example usage:
//     void Assert(const char* pExpression, void* pContext);
//     SetAssertionFailureFunction(Assert, this);
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_ASSERTION_FAILURE_DEFINED
    #define EASTL_ASSERTION_FAILURE_DEFINED

    namespace eastl
    {
        typedef void (*EASTL_AssertionFailureFunction)(const char* pExpression, void* pContext);
        EASTL_API void SetAssertionFailureFunction(EASTL_AssertionFailureFunction pFunction, void* pContext);

        // These are the internal default functions that implement asserts.
        EASTL_API void AssertionFailure(const char* pExpression);
        EASTL_API void AssertionFailureFunctionDefault(const char* pExpression, void* pContext);
    }
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_ASSERT
//
// Assertion macro. Can be overridden by user with a different value.
//
// Example usage:
//    EASTL_ASSERT(intVector.size() < 100);
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_ASSERT
    #if EASTL_ASSERT_ENABLED
        #define EASTL_ASSERT(expression) (void)((expression) || (eastl::AssertionFailure(#expression), 0))
    #else
        #define EASTL_ASSERT(expression)
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_FAIL_MSG
//
// Failure macro. Can be overridden by user with a different value.
//
// Example usage:
//    EASTL_FAIL("detected error condition!");
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_FAIL_MSG
    #if EASTL_ASSERT_ENABLED
        #define EASTL_FAIL_MSG(message) (eastl::AssertionFailure(message))
    #else
        #define EASTL_FAIL_MSG(message)
    #endif
#endif




///////////////////////////////////////////////////////////////////////////////
// EASTL_CT_ASSERT / EASTL_CT_ASSERT_NAMED
//
// EASTL_CT_ASSERT is a macro for compile time assertion checks, useful for 
// validating *constant* expressions. The advantage over using EASTL_ASSERT 
// is that errors are caught at compile time instead of runtime.
//
// Example usage:
//     EASTL_CT_ASSERT(sizeof(uint32_t == 4));
//
///////////////////////////////////////////////////////////////////////////////

#if !defined(EASTL_CT_ASSERT)
#if defined(EASTL_DEBUG)
    template <bool>  struct EASTL_CT_ASSERTION_FAILURE;
    template <>      struct EASTL_CT_ASSERTION_FAILURE<true>{ enum { value = 1 }; }; // We create a specialization for true, but not for false.
    template <int x> struct EASTL_CT_ASSERTION_TEST{};

    #define EASTL_PREPROCESSOR_JOIN(a, b)  EASTL_PREPROCESSOR_JOIN1(a, b)
    #define EASTL_PREPROCESSOR_JOIN1(a, b) EASTL_PREPROCESSOR_JOIN2(a, b)
    #define EASTL_PREPROCESSOR_JOIN2(a, b) a##b

    #if defined(_MSC_VER)
        #define EASTL_CT_ASSERT(expression)  typedef EASTL_CT_ASSERTION_TEST< sizeof(EASTL_CT_ASSERTION_FAILURE< (bool)(expression) >)> EASTL_CT_ASSERT_FAILURE
    #elif defined(__ICL) || defined(__ICC)
        #define EASTL_CT_ASSERT(expression)  typedef char EASTL_PREPROCESSOR_JOIN(EASTL_CT_ASSERT_FAILURE_, __LINE__) [EASTL_CT_ASSERTION_FAILURE< (bool)(expression) >::value]
    #elif defined(__MWERKS__)
        #define EASTL_CT_ASSERT(expression)  enum { EASTL_PREPROCESSOR_JOIN(EASTL_CT_ASSERT_FAILURE_, __LINE__) = sizeof(EASTL_CT_ASSERTION_FAILURE< (bool)(expression) >) }
    #else // GCC, etc.
        //#define EASTL_CT_ASSERT(expression)  typedef EASTL_CT_ASSERTION_TEST< sizeof(EASTL_CT_ASSERTION_FAILURE< (bool)(expression) >)> EASTL_PREPROCESSOR_JOIN1(EASTL_CT_ASSERT_FAILURE_, __LINE__)
    #endif
#else
    #define EASTL_CT_ASSERT(expression)
#endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_DEBUG_BREAK
//
// This function causes an app to immediately stop under the debugger.
// It is implemented as a macro in order to allow stopping at the site 
// of the call.
//
//
// Example usage:
//     EASTL_DEBUG_BREAK();
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_DEBUG_BREAK
    #if defined(_MSC_VER) && (_MSC_VER >= 1300)
        #define EASTL_DEBUG_BREAK() __debugbreak()    // This is a compiler intrinsic which will map to appropriate inlined asm for the platform.
    #elif defined(EA_PROCESSOR_MIPS)                  // 
        #define EASTL_DEBUG_BREAK() asm("break")
    #elif defined(__SNC__)
        #define EASTL_DEBUG_BREAK() *(int*)(0) = 0
    #elif defined(EA_PLATFORM_PS3)
        #define EASTL_DEBUG_BREAK() asm volatile("tw 31,1,1")
    #elif defined(EA_PROCESSOR_POWERPC)               // Generic PowerPC. 
        #define EASTL_DEBUG_BREAK() asm(".long 0")    // This triggers an exception by executing opcode 0x00000000.
    #elif (defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64)) && defined(EA_ASM_STYLE_INTEL)
        #define EASTL_DEBUG_BREAK() { __asm int 3 }
    #elif (defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64)) && (defined(EA_ASM_STYLE_ATT) || defined(__GNUC__))
        #define EASTL_DEBUG_BREAK() asm("int3") 
    #else
        void EASTL_DEBUG_BREAK(); // User must define this externally.
    #endif
#else
    void EASTL_DEBUG_BREAK(); // User must define this externally.
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_ALLOCATOR_COPY_ENABLED
//
// Defined as 0 or 1. Default is 0 (disabled) until some future date.
// If enabled (1) then container operator= copies the allocator from the 
// source container. It ideally should be set to enabled but for backwards
// compatibility with older versions of EASTL it is currently set to 0.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_ALLOCATOR_COPY_ENABLED
    #define EASTL_ALLOCATOR_COPY_ENABLED 1
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_FIXED_SIZE_TRACKING_ENABLED
//
// Defined as an integer >= 0. Default is same as EASTL_DEBUG.
// If EASTL_FIXED_SIZE_TRACKING_ENABLED is enabled, then fixed
// containers in debug builds track the max count of objects 
// that have been in the container. This allows for the tuning
// of fixed container sizes to their minimum required size.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_FIXED_SIZE_TRACKING_ENABLED
    #define EASTL_FIXED_SIZE_TRACKING_ENABLED EASTL_DEBUG
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_RTTI_ENABLED
//
// Defined as 0 or 1. Default is 1 if RTTI is supported by the compiler.
// This define exists so that we can use some dynamic_cast operations in the 
// code without warning. dynamic_cast is only used if the specifically refers
// to it; EASTL won't do dynamic_cast behind your back.
//
// Example usage:
//     #if EASTL_RTTI_ENABLED
//         pChildClass = dynamic_cast<ChildClass*>(pParentClass);
//     #endif
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_RTTI_ENABLED
    #if defined(EA_COMPILER_NO_RTTI)
        #define EASTL_RTTI_ENABLED 0
    #else
        #define EASTL_RTTI_ENABLED 1
    #endif
#endif




///////////////////////////////////////////////////////////////////////////////
// EASTL_EXCEPTIONS_ENABLED
//
// Defined as 0 or 1. Default is to follow what the compiler settings are.
// The user can predefine EASTL_EXCEPTIONS_ENABLED to 0 or 1; however, if the 
// compiler is set to disable exceptions then EASTL_EXCEPTIONS_ENABLED is 
// forced to a value of 0 regardless of the user predefine.
//
///////////////////////////////////////////////////////////////////////////////

#if !defined(EASTL_EXCEPTIONS_ENABLED) || ((EASTL_EXCEPTIONS_ENABLED == 1) && defined(EA_COMPILER_NO_EXCEPTIONS))
    #define EASTL_EXCEPTIONS_ENABLED 0
#endif





///////////////////////////////////////////////////////////////////////////////
// EASTL_STRING_OPT_XXXX
//
// Enables some options / optimizations options that cause the string class 
// to behave slightly different from the C++ standard basic_string. These are 
// options whereby you can improve performance by avoiding operations that 
// in practice may never occur for you.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_STRING_OPT_CHAR_INIT
    // Defined as 0 or 1. Default is 1.
    // Defines if newly created characters are initialized to 0 or left
    // as random values.
    // The C++ string standard is to initialize chars to 0.
    #define EASTL_STRING_OPT_CHAR_INIT 1
#endif

#ifndef EASTL_STRING_OPT_EXPLICIT_CTORS
    // Defined as 0 or 1. Default is 0.
    // Defines if we should implement explicity in constructors where the C++
    // standard string does not. The advantage of enabling explicit constructors
    // is that you can do this: string s = "hello"; in addition to string s("hello");
    // The disadvantage of enabling explicity constructors is that there can be 
    // silent conversions done which impede performance if the user isn't paying
    // attention.
    // C++ standard string ctors are not explicit.
    #define EASTL_STRING_OPT_EXPLICIT_CTORS 0
#endif

#ifndef EASTL_STRING_OPT_LENGTH_ERRORS
    // Defined as 0 or 1. Default is equal to EASTL_EXCEPTIONS_ENABLED.
    // Defines if we check for string values going beyond kMaxSize 
    // (a very large value) and throw exections if so.
    // C++ standard strings are expected to do such checks.
    #define EASTL_STRING_OPT_LENGTH_ERRORS EASTL_EXCEPTIONS_ENABLED
#endif

#ifndef EASTL_STRING_OPT_RANGE_ERRORS
    // Defined as 0 or 1. Default is equal to EASTL_EXCEPTIONS_ENABLED.
    // Defines if we check for out-of-bounds references to string
    // positions and throw exceptions if so. Well-behaved code shouldn't 
    // refence out-of-bounds positions and so shouldn't need these checks.
    // C++ standard strings are expected to do such range checks.
    #define EASTL_STRING_OPT_RANGE_ERRORS EASTL_EXCEPTIONS_ENABLED
#endif

#ifndef EASTL_STRING_OPT_ARGUMENT_ERRORS
    // Defined as 0 or 1. Default is 0.
    // Defines if we check for NULL ptr arguments passed to string 
    // functions by the user and throw exceptions if so. Well-behaved code 
    // shouldn't pass bad arguments and so shouldn't need these checks.
    // Also, some users believe that strings should check for NULL pointers 
    // in all their arguments and do no-ops if so. This is very debatable.
    // C++ standard strings are not required to check for such argument errors.
    #define EASTL_STRING_OPT_ARGUMENT_ERRORS 0
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_ABSTRACT_STRING_ENABLED
//
// Defined as 0 or 1. Default is 0 until abstract string is fully tested.
// Defines whether the proposed replacement for the string module is enabled.
// See bonus/abstract_string.h for more information.
//
#ifndef EASTL_ABSTRACT_STRING_ENABLED
    #define EASTL_ABSTRACT_STRING_ENABLED 0
#endif




///////////////////////////////////////////////////////////////////////////////
// EASTL_BITSET_SIZE_T
//
// Defined as 0 or 1. Default is 1.
// Controls whether bitset uses size_t or eastl_size_t.
//
#ifndef EASTL_BITSET_SIZE_T
    #define EASTL_BITSET_SIZE_T 1
#endif




///////////////////////////////////////////////////////////////////////////////
// EASTL_LIST_SIZE_CACHE
//
// Defined as 0 or 1. Default is 0.
// If defined as 1, the list and slist containers (and possibly any additional
// containers as well) keep a member mSize (or similar) variable which allows
// the size() member function to execute in constant time (a.k.a. O(1)). 
// There are debates on both sides as to whether it is better to have this 
// cached value or not, as having it entails some cost (memory and code).
// To consider: Make list size caching an optional template parameter.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_LIST_SIZE_CACHE
    #define EASTL_LIST_SIZE_CACHE 0
#endif

#ifndef EASTL_SLIST_SIZE_CACHE
    #define EASTL_SLIST_SIZE_CACHE 0
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_MAX_STACK_USAGE
//
// Defined as an integer greater than zero. Default is 4000.
// There are some places in EASTL where temporary objects are put on the 
// stack. A common example of this is in the implementation of container
// swap functions whereby a temporary copy of the container is made.
// There is a problem, however, if the size of the item created on the stack 
// is very large. This can happen with fixed-size containers, for example.
// The EASTL_MAX_STACK_USAGE define specifies the maximum amount of memory
// (in bytes) that the given platform/compiler will safely allow on the stack.
// Platforms such as Windows will generally allow larger values than embedded
// systems or console machines, but it is usually a good idea to stick with
// a max usage value that is portable across all platforms, lest the user be
// surprised when something breaks as it is ported to another platform.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_MAX_STACK_USAGE
    #define EASTL_MAX_STACK_USAGE 4000
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_VA_COPY_ENABLED
//
// Defined as 0 or 1. Default is 1 for compilers that need it, 0 for others.
// Some compilers on some platforms implement va_list whereby its contents  
// are destroyed upon usage, even if passed by value to another function. 
// With these compilers you can use va_copy to restore the a va_list.
// Known compiler/platforms that destroy va_list contents upon usage include:
//     CodeWarrior on PowerPC
//     GCC on x86-64
// However, va_copy is part of the C99 standard and not part of earlier C and
// C++ standards. So not all compilers support it. VC++ doesn't support va_copy,
// but it turns out that VC++ doesn't need it on the platforms it supports.
// For example usage, see the EASTL string.h file.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_VA_COPY_ENABLED
    #if defined(__MWERKS__) || (defined(__GNUC__) && (__GNUC__ >= 3) && (!defined(__i386__) || defined(__x86_64__)) && !defined(__ppc__) && !defined(__PPC__) && !defined(__PPC64__))
        #define EASTL_VA_COPY_ENABLED 1
    #else
        #define EASTL_VA_COPY_ENABLED 0
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_LIST_PROXY_ENABLED
//
#if !defined(EASTL_LIST_PROXY_ENABLED)
    // GCC with -fstrict-aliasing has bugs (or undocumented functionality in their 
    // __may_alias__ implementation. The compiler gets confused about function signatures.
    // VC8 (1400) doesn't need the proxy because it has built-in smart debugging capabilities.
    #if defined(EASTL_DEBUG) && (!defined(__GNUC__) || defined(__SNC__)) && (!defined(_MSC_VER) || (_MSC_VER < 1400))
        #define EASTL_LIST_PROXY_ENABLED 1
        #define EASTL_LIST_PROXY_MAY_ALIAS EASTL_MAY_ALIAS
    #else
        #define EASTL_LIST_PROXY_ENABLED 0
        #define EASTL_LIST_PROXY_MAY_ALIAS
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_STD_ITERATOR_CATEGORY_ENABLED
//
// Defined as 0 or 1. Default is 1.
// If defined as non-zero, EASTL iterator categories (iterator.h's input_iterator_tag,
// forward_iterator_tag, etc.) are defined to be those from std C++ in the std 
// namespace. The reason for wanting to enable such a feature is that it allows 
// EASTL containers and algorithms to work with std STL containes and algorithms.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_STD_ITERATOR_CATEGORY_ENABLED
//    #define EASTL_STD_ITERATOR_CATEGORY_ENABLED 1
#endif

#if EASTL_STD_ITERATOR_CATEGORY_ENABLED
    #define EASTL_ITC_NS std
#else
    #define EASTL_ITC_NS eastl
#endif




///////////////////////////////////////////////////////////////////////////////
// EASTL_VALIDATION_ENABLED
//
// Defined as an integer >= 0. Default is to be equal to EASTL_DEBUG.
// If nonzero, then a certain amount of automatic runtime validation is done.
// Runtime validation is not considered the same thing as asserting that user
// input values are valid. Validation refers to internal consistency checking
// of the validity of containers and their iterators. Validation checking is
// something that often involves significantly more than basic assertion 
// checking, and it may sometimes be desirable to disable it.
// This macro would generally be used internally by EASTL.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_VALIDATION_ENABLED
    #define EASTL_VALIDATION_ENABLED EASTL_DEBUG
#endif




///////////////////////////////////////////////////////////////////////////////
// EASTL_VALIDATE_COMPARE
//
// Defined as EASTL_ASSERT or defined away. Default is EASTL_ASSERT if EASTL_VALIDATION_ENABLED is enabled.
// This is used to validate user-supplied comparison functions, particularly for sorting purposes.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_VALIDATE_COMPARE_ENABLED
    #define EASTL_VALIDATE_COMPARE_ENABLED EASTL_VALIDATION_ENABLED
#endif

#if EASTL_VALIDATE_COMPARE_ENABLED
    #define EASTL_VALIDATE_COMPARE EASTL_ASSERT
#else
    #define EASTL_VALIDATE_COMPARE(expression)
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_VALIDATE_INTRUSIVE_LIST
//
// Defined as an integral value >= 0. Controls the amount of automatic validation
// done by intrusive_list. A value of 0 means no automatic validation is done.
// As of this writing, EASTL_VALIDATE_INTRUSIVE_LIST defaults to 0, as it makes
// the intrusive_list_node become a non-POD, which may be an issue for some code.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_VALIDATE_INTRUSIVE_LIST
    #define EASTL_VALIDATE_INTRUSIVE_LIST 0
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_FORCE_INLINE
//
// Defined as a "force inline" expression or defined away.
// You generally don't need to use forced inlining with the Microsoft and 
// Metrowerks compilers, but you may need it with the GCC compiler (any version).
// 
// Example usage:
//     template <typename T, typename Allocator>
//     EASTL_FORCE_INLINE typename vector<T, Allocator>::size_type
//     vector<T, Allocator>::size() const
//        { return mpEnd - mpBegin; }
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_FORCE_INLINE
    #define EASTL_FORCE_INLINE EA_FORCE_INLINE
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_MAY_ALIAS
//
// Defined as a macro that wraps the GCC may_alias attribute. This attribute
// has no significance for VC++ because VC++ doesn't support the concept of 
// strict aliasing. Users should avoid writing code that breaks strict 
// aliasing rules; EASTL_MAY_ALIAS is for cases with no alternative.
//
// Example usage:
//    uint32_t value EASTL_MAY_ALIAS;
//
// Example usage:
//    typedef uint32_t EASTL_MAY_ALIAS value_type;
//    value_type value;
//
#if defined(__GNUC__) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 303)
    #define EASTL_MAY_ALIAS __attribute__((__may_alias__))
#else
    #define EASTL_MAY_ALIAS
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_LIKELY / EASTL_UNLIKELY
//
// Defined as a macro which gives a hint to the compiler for branch
// prediction. GCC gives you the ability to manually give a hint to 
// the compiler about the result of a comparison, though it's often
// best to compile shipping code with profiling feedback under both
// GCC (-fprofile-arcs) and VC++ (/LTCG:PGO, etc.). However, there 
// are times when you feel very sure that a boolean expression will
// usually evaluate to either true or false and can help the compiler
// by using an explicity directive...
//
// Example usage:
//    if(EASTL_LIKELY(a == 0)) // Tell the compiler that a will usually equal 0.
//       { ... }
//
// Example usage:
//    if(EASTL_UNLIKELY(a == 0)) // Tell the compiler that a will usually not equal 0.
//       { ... }
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_LIKELY
    #if defined(__GNUC__) && (__GNUC__ >= 3)
        #define EASTL_LIKELY(x)   __builtin_expect(!!(x), true)
        #define EASTL_UNLIKELY(x) __builtin_expect(!!(x), false) 
    #else
        #define EASTL_LIKELY(x)   (x)
        #define EASTL_UNLIKELY(x) (x)
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_MINMAX_ENABLED
//
// Defined as 0 or 1; default is 1.
// Specifies whether the min and max algorithms are available. 
// It may be useful to disable the min and max algorithems because sometimes
// #defines for min and max exist which would collide with EASTL min and max.
// Note that there are already alternative versions of min and max in EASTL
// with the min_alt and max_alt functions. You can use these without colliding
// with min/max macros that may exist.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef EASTL_MINMAX_ENABLED
    #define EASTL_MINMAX_ENABLED 1
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_NOMINMAX
//
// Defined as 0 or 1; default is 1.
// MSVC++ has #defines for min/max which collide with the min/max algorithm
// declarations. If EASTL_NOMINMAX is defined as 1, then we undefine min and 
// max if they are #defined by an external library. This allows our min and 
// max definitions in algorithm.h to work as expected. An alternative to 
// the enabling of EASTL_NOMINMAX is to #define NOMINMAX in your project 
// settings if you are compiling for Windows.
// Note that this does not control the availability of the EASTL min and max 
// algorithms; the EASTL_MINMAX_ENABLED configuration parameter does that.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_NOMINMAX
    #define EASTL_NOMINMAX 1
#endif




///////////////////////////////////////////////////////////////////////////////
// EASTL_ALIGN_OF
//
// Determines the alignment of a type.
//
// Example usage:
//    size_t alignment = EASTL_ALIGN_OF(int);
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_ALIGN_OF
    #if defined(__MWERKS__)
          #define EASTL_ALIGN_OF(type) ((size_t)__alignof__(type))
    #elif !defined(__GNUC__) || (__GNUC__ >= 3) // GCC 2.x doesn't do __alignof correctly all the time.
        #define EASTL_ALIGN_OF __alignof
    #else
        #define EASTL_ALIGN_OF(type) ((size_t)offsetof(struct{ char c; type m; }, m))
    #endif
#endif




///////////////////////////////////////////////////////////////////////////////
// eastl_size_t
//
// Defined as an unsigned integer type, usually either size_t or uint32_t.
// Defaults to uint32_t instead of size_t because the latter wastes memory 
// and is sometimes slower on 64 bit machines.
//
// Example usage:
//     eastl_size_t n = intVector.size();
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_SIZE_T
    #if(EA_PLATFORM_WORD_SIZE == 4) // If (sizeof(size_t) == 4) and we can thus use size_t as-is...
        #include <stddef.h>
        #define EASTL_SIZE_T  size_t
        #define EASTL_SSIZE_T intptr_t
    #else
        #define EASTL_SIZE_T  uint32_t
        #define EASTL_SSIZE_T int32_t
    #endif
#endif

typedef EASTL_SIZE_T  eastl_size_t;  // Same concept as std::size_t.
typedef EASTL_SSIZE_T eastl_ssize_t; // Signed version of eastl_size_t. Concept is similar to Posix's ssize_t.






///////////////////////////////////////////////////////////////////////////////
// AddRef / Release
//
// AddRef and Release are used for "intrusive" reference counting. By the term
// "intrusive", we mean that the reference count is maintained by the object 
// and not by the user of the object. Given that an object implements referencing 
// counting, the user of the object needs to be able to increment and decrement
// that reference count. We do that via the venerable AddRef and Release functions
// which the object must supply. These defines here allow us to specify the name
// of the functions. They could just as well be defined to addref and delref or 
// IncRef and DecRef.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTLAddRef
    #define EASTLAddRef AddRef
#endif

#ifndef EASTLRelease
    #define EASTLRelease Release
#endif




///////////////////////////////////////////////////////////////////////////////
// EASTL_ALLOCATOR_EXPLICIT_ENABLED
//
// Defined as 0 or 1. Default is 0 for now but ideally would be changed to 
// 1 some day. It's 0 because setting it to 1 breaks some existing code.
// This option enables the allocator ctor to be explicit, which avoids
// some undesirable silent conversions, especially with the string class.
//
// Example usage:
//     class allocator
//     {
//     public:
//         EASTL_ALLOCATOR_EXPLICIT allocator(const char* pName);
//     };
//
///////////////////////////////////////////////////////////////////////////////

//#define EASTL_USER_DEFINED_ALLOCATOR 1

#ifndef EASTL_ALLOCATOR_EXPLICIT_ENABLED
    #define EASTL_ALLOCATOR_EXPLICIT_ENABLED 0
#endif

#if EASTL_ALLOCATOR_EXPLICIT_ENABLED
    #define EASTL_ALLOCATOR_EXPLICIT explicit
#else
    #define EASTL_ALLOCATOR_EXPLICIT 
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL allocator
//
// The EASTL allocator system allows you to redefine how memory is allocated
// via some defines that are set up here. In the container code, memory is 
// allocated via macros which expand to whatever the user has them set to 
// expand to. Given that there are multiple allocator systems available, 
// this system allows you to configure it to use whatever system you want,
// provided your system meets the requirements of this library. 
// The requirements are:
//
//     - Must be constructable via a const char* (name) parameter.
//       Some uses of allocators won't require this, however.
//     - Allocate a block of memory of size n and debug name string.
//     - Allocate a block of memory of size n, debug name string, 
//       alignment a, and offset o.
//     - Free memory allocated via either of the allocation functions above.
//     - Provide a default allocator instance which can be used if the user 
//       doesn't provide a specific one.
//
///////////////////////////////////////////////////////////////////////////////

// namespace eastl
// {
//     class allocator
//     {
//         allocator(const char* pName = NULL);
//
//         void* allocate(size_t n, int flags = 0);
//         void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0);
//         void  deallocate(void* p, size_t n);
//
//         const char* get_name() const;
//         void        set_name(const char* pName);
//     };
//
//     allocator* GetDefaultAllocator(); // This is used for anonymous allocations.
// }

#ifndef EASTLAlloc // To consider: Instead of calling through pAllocator, just go directly to operator new, since that's what allocator does.
    #define EASTLAlloc(allocator, n) (allocator).allocate(n);
#endif

#ifndef EASTLAllocFlags // To consider: Instead of calling through pAllocator, just go directly to operator new, since that's what allocator does.
    #define EASTLAllocFlags(allocator, n, flags) (allocator).allocate(n, flags);
#endif

#ifndef EASTLAllocAligned
    #define EASTLAllocAligned(allocator, n, alignment, offset) (allocator).allocate((n), (alignment), (offset))
#endif

#ifndef EASTLFree
    #define EASTLFree(allocator, p, size) (allocator).deallocate((p), (size))
#endif

#ifndef EASTLAllocatorType
    #define EASTLAllocatorType eastl::allocator
#endif

#ifndef EASTLAllocatorDefault
    // EASTLAllocatorDefault returns the default allocator instance. This is not a global 
    // allocator which implements all container allocations but is the allocator that is 
    // used when EASTL needs to allocate memory internally. There are very few cases where 
    // EASTL allocates memory internally, and in each of these it is for a sensible reason 
    // that is documented to behave as such.
    #define EASTLAllocatorDefault eastl::GetDefaultAllocator
#endif








#endif // Header include guard







