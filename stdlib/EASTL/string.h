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
// EASTL/string.h
// Written and maintained by Paul Pedriana - 2005.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Implements a basic_string class, much like the C++ std::basic_string.
// The primary distinctions between basic_string and std::basic_string are:
//    - basic_string has a few extension functions that allow for increased performance.
//    - basic_string has a few extension functions that make use easier, 
//      such as a member sprintf function and member tolower/toupper functions.
//    - basic_string supports debug memory naming natively.
//    - basic_string is easier to read, debug, and visualize.
//    - basic_string internally manually expands basic functions such as begin(),
//      size(), etc. in order to improve debug performance and optimizer success.
//    - basic_string is savvy to an environment that doesn't have exception handling,
//      as is sometimes the case with console or embedded environments.
//    - basic_string has less deeply nested function calls and allows the user to 
//      enable forced inlining in debug builds in order to reduce bloat.
//    - basic_string doesn't use char traits. As a result, EASTL assumes that 
//      strings will hold characters and not exotic things like widgets. At the 
//      very least, basic_string assumes that the value_type is a POD.
//    - basic_string::size_type is defined as eastl_size_t instead of size_t in 
//      order to save memory and run faster on 64 bit systems.
//    - basic_string data is guaranteed to be contiguous.
//    - basic_string data is guaranteed to be 0-terminated, and the c_str() function
//      is guaranteed to return the same pointer as the data() which is guaranteed
//      to be the same value as &string[0].
//    - basic_string has a set_capacity() function which frees excess capacity. 
//      The only way to do this with std::basic_string is via the cryptic non-obvious 
//      trick of using: basic_string<char>(x).swap(x);
//    - basic_string has a force_size() function, which unilaterally moves the string 
//      end position (mpEnd) to the given location. Useful for when the user writes 
//      into the string via some extenal means such as C strcpy or sprintf.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Copy on Write (cow)
//
// This string implementation does not do copy on write (cow). This is by design,
// as cow penalizes 95% of string uses for the benefit of only 5% of the uses 
// (these percentages are qualitative, not quantitative). The primary benefit of
// cow is that it allows for the sharing of string data between two string objects.
// Thus if you say this:
//    string a("hello");
//    string b(a);
// the "hello" will be shared between a and b. If you then say this:
//    a = "world";
// then a will release its reference to "hello" and leave b with the only reference
// to it. Normally this functionality is accomplished via reference counting and 
// with atomic operations or mutexes.
//
// The C++ standard does not say anything about basic_string and cow. However, 
// for a basic_string implementation to be standards-conforming, a number of
// issues arise which dictate some things about how one would have to implement
// a cow string. The discussion of these issues will not be rehashed here, as you
// can read the references below for better detail than can be provided in the 
// space we have here. However, we can say that the C++ standard is sensible and 
// that anything we try to do here to allow for an efficient cow implementation
// would result in a generally unacceptable string interface.
//
// The disadvantages of cow strings are:
//    - A reference count needs to exist with the string, which increases string memory usage.
//    - With thread safety, atomic operations and mutex locks are expensive, especially 
//      on weaker memory systems such as console gaming platforms.
//    - All non-const string accessor functions need to do a sharing check the the 
//      first such check needs to detach the string. Similarly, all string assignments 
//      need to do a sharing check as well. If you access the string before doing an 
//      assignment, the assignment doesn't result in a shared string, because the string 
//      has already been detached.
//    - String sharing doesn't happen the large majority of the time. In some cases, 
//      the total sum of the reference count memory can exceed any memory savings 
//      gained by the strings that share representations.  
// 
// The addition of a string_cow class is under consideration for this library. 
// There are conceivably some systems which have string usage patterns which would
// benefit from cow sharing. Such functionality is best saved for a separate string 
// implementation so that the other string uses aren't penalized.
// 
// References:
//    This is a good starting HTML reference on the topic:
//       http://www.gotw.ca/publications/optimizations.htm
//    Here is a Usenet discussion on the topic:
//       http://groups-beta.google.com/group/comp.lang.c++.moderated/browse_thread/thread/3dc6af5198d0bf7/886c8642cb06e03d
//
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_STRING_H
#define EASTL_STRING_H


#include <EASTL/internal/config.h>
#if EASTL_ABSTRACT_STRING_ENABLED
    #include <EASTL/bonus/string_abstract.h>
#else // 'else' encompasses the entire rest of this file.
#include <EASTL/allocator.h>
#include <EASTL/iterator.h>
#include <EASTL/algorithm.h>
#ifdef __clang__
    #include <EASTL/internal/hashtable.h>
#endif

#ifdef _MSC_VER
    #pragma warning(push, 0)
#endif
#include <stddef.h>             // size_t, ptrdiff_t, etc.
#include <stdarg.h>             // vararg functionality.
#include <stdlib.h>             // malloc, free.
#include <stdio.h>              // snprintf, etc.
#include <ctype.h>              // toupper, etc.
#include <wchar.h>              // toupper, etc.
#ifdef __MWERKS__
    #include <../Include/string.h> // Force the compiler to use the std lib header.
#else
    #include <string.h> // strlen, etc.
#endif
#ifdef _MSC_VER
    #pragma warning(pop)
#endif

#if EASTL_EXCEPTIONS_ENABLED
    #ifdef _MSC_VER
        #pragma warning(push, 0)
    #endif
    #include <stdexcept> // std::out_of_range, std::length_error.
    #ifdef _MSC_VER
        #pragma warning(pop)
    #endif
#endif

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4530)  // C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
    #pragma warning(disable: 4267)  // 'argument' : conversion from 'size_t' to 'const uint32_t', possible loss of data. This is a bogus warning resulting from a bug in VC++.
    #pragma warning(disable: 4480)  // nonstandard extension used: specifying underlying type for enum
#endif


///////////////////////////////////////////////////////////////////////////////
// EASTL_STRING_EXPLICIT
//
// See EASTL_STRING_OPT_EXPLICIT_CTORS for documentation.
//
#if EASTL_STRING_OPT_EXPLICIT_CTORS
    #define EASTL_STRING_EXPLICIT explicit
#else
    #define EASTL_STRING_EXPLICIT
#endif
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
// EASTL_STRING_INITIAL_CAPACITY
//
// As of this writing, this must be > 0. Note that an initially empty string 
// has a capacity of zero (it allocates no memory).
//
const eastl_size_t EASTL_STRING_INITIAL_CAPACITY = 8;
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Vsnprintf8 / Vsnprintf16
//
// The user is expected to supply these functions. Note that these functions
// are expected to accept parameters as per the C99 standard. These functions
// can deal with C99 standard return values or Microsoft non-standard return
// values but act more efficiently if implemented via the C99 style.

extern int Vsnprintf8 (char8_t*  pDestination, size_t n, const char8_t*  pFormat, va_list arguments);
extern int Vsnprintf16(char16_t* pDestination, size_t n, const char16_t* pFormat, va_list arguments);
extern int Vsnprintf32(char32_t* pDestination, size_t n, const char32_t* pFormat, va_list arguments);

namespace eastl
{
    inline int Vsnprintf(char8_t* pDestination, size_t n, const char8_t* pFormat, va_list arguments)
        { return Vsnprintf8(pDestination, n, pFormat, arguments); }

    inline int Vsnprintf(char16_t* pDestination, size_t n, const char16_t* pFormat, va_list arguments)
        { return Vsnprintf16(pDestination, n, pFormat, arguments); }

    inline int Vsnprintf(char32_t* pDestination, size_t n, const char32_t* pFormat, va_list arguments)
        { return Vsnprintf32(pDestination, n, pFormat, arguments); }
}
///////////////////////////////////////////////////////////////////////////////



namespace eastl
{

    /// EASTL_BASIC_STRING_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    ///
    #ifndef EASTL_BASIC_STRING_DEFAULT_NAME
        #define EASTL_BASIC_STRING_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " basic_string" // Unless the user overrides something, this is "EASTL basic_string".
    #endif


    /// EASTL_BASIC_STRING_DEFAULT_ALLOCATOR
    ///
    #ifndef EASTL_BASIC_STRING_DEFAULT_ALLOCATOR
        #define EASTL_BASIC_STRING_DEFAULT_ALLOCATOR allocator_type(EASTL_BASIC_STRING_DEFAULT_NAME)
    #endif



    /// gEmptyString
    ///
    /// Declares a shared terminating 0 representation for scalar strings that are empty.
    ///
    union EmptyString
    {
        uint32_t       mUint32;
        char           mEmpty8[1];
        unsigned char  mEmptyU8[1];
        signed char    mEmptyS8[1];
        char16_t       mEmpty16[1];
        char32_t       mEmpty32[1];
    };
    extern EASTL_API EmptyString gEmptyString;

    inline const signed char*   GetEmptyString(signed char)   { return gEmptyString.mEmptyS8;  }
    inline const unsigned char* GetEmptyString(unsigned char) { return gEmptyString.mEmptyU8;  }
    inline const char*          GetEmptyString(char)          { return gEmptyString.mEmpty8;  }
    inline const char16_t*      GetEmptyString(char16_t)      { return gEmptyString.mEmpty16; }
    inline const char32_t*      GetEmptyString(char32_t)      { return gEmptyString.mEmpty32; }


    ///////////////////////////////////////////////////////////////////////////////
    /// basic_string
    ///
    /// Implements a templated string class, somewhat like C++ std::basic_string.
    ///
    /// Notes: 
    ///     As of this writing, an insert of a string into itself necessarily
    ///     triggers a reallocation, even if there is enough capacity in self
    ///     to handle the increase in size. This is due to the slightly tricky 
    ///     nature of the operation of modifying one's self with one's self,
    ///     and thus the source and destination are being modified during the
    ///     operation. It might be useful to rectify this to the extent possible.
    ///
    template <typename T, typename Allocator = EASTLAllocatorType>
    class basic_string
    {
    public:
        typedef basic_string<T, Allocator>                      this_type;
        typedef T                                               value_type;
        typedef T*                                              pointer;
        typedef const T*                                        const_pointer;
        typedef T&                                              reference;
        typedef const T&                                        const_reference;
        typedef T*                                              iterator;           // Maintainer note: We want to leave iterator defined as T* -- at least in release builds -- as this gives some algorithms an advantage that optimizers cannot get around.
        typedef const T*                                        const_iterator;
        typedef eastl::reverse_iterator<iterator>               reverse_iterator;
        typedef eastl::reverse_iterator<const_iterator>         const_reverse_iterator;
        typedef eastl_size_t                                    size_type;          // See config.h for the definition of eastl_size_t, which defaults to uint32_t.
        typedef ptrdiff_t                                       difference_type;
        typedef Allocator                                       allocator_type;

        #if defined(_MSC_VER) && (_MSC_VER >= 1400) // _MSC_VER of 1400 means VC8 (VS2005), 1500 means VC9 (VS2008)
            enum : size_type {                      // Use Microsoft enum language extension, allowing for smaller debug symbols than using a static const. Users have been affected by this.
                npos     = (size_type)-1,
                kMaxSize = (size_type)-2
            };
        #else
            static const size_type npos     = (size_type)-1;      /// 'npos' means non-valid position or simply non-position.
            static const size_type kMaxSize = (size_type)-2;      /// -1 is reserved for 'npos'. It also happens to be slightly beneficial that kMaxSize is a value less than -1, as it helps us deal with potential integer wraparound issues.
        #endif

        enum
        {
            kAlignment       = EASTL_ALIGN_OF(T),
            kAlignmentOffset = 0
        };

    public:
        // CtorDoNotInitialize exists so that we can create a constructor that allocates but doesn't 
        // initialize and also doesn't collide with any other constructor declaration.
        struct CtorDoNotInitialize{};

        // CtorSprintf exists so that we can create a constructor that accepts printf-style  
        // arguments but also doesn't collide with any other constructor declaration.
        struct CtorSprintf{};

    protected:
        value_type*       mpBegin;      // Begin of string.
        value_type*       mpEnd;        // End of string. *mpEnd is always '0', as we 0-terminate our string. mpEnd is always < mpCapacity.
        value_type*       mpCapacity;   // End of allocated space, including the space needed to store the trailing '0' char. mpCapacity is always at least mpEnd + 1.
        allocator_type    mAllocator;   // To do: Use base class optimization to make this go away.

    public:
        // Constructor, destructor
        basic_string();
        explicit basic_string(const allocator_type& allocator);
        basic_string(const this_type& x, size_type position, size_type n = npos);
        basic_string(const value_type* p, size_type n, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR);
        EASTL_STRING_EXPLICIT basic_string(const value_type* p, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR);
        basic_string(size_type n, value_type c, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR);
        basic_string(const this_type& x);
#ifdef EA_COMPILER_HAS_MOVE_SEMANTICS
        basic_string(this_type&& x);
#endif
        basic_string(const value_type* pBegin, const value_type* pEnd, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR);
        basic_string(CtorDoNotInitialize, size_type n, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR);
        basic_string(CtorSprintf, const value_type* pFormat, ...);

       ~basic_string();

        // Allocator
        const allocator_type& get_allocator() const;
        allocator_type&       get_allocator();
        void                  set_allocator(const allocator_type& allocator);

        // Operator =
        this_type& operator=(const this_type& x);
        this_type& operator=(const value_type* p);
        this_type& operator=(value_type c);

        void          swap(this_type& x);

        // Assignment operations
        basic_string& assign(const basic_string& x);
        basic_string& assign(const basic_string& x, size_type position, size_type n);
        basic_string& assign(const value_type* p, size_type n);
        basic_string& assign(const value_type* p);
        basic_string& assign(size_type n, value_type c);
        basic_string& assign(const value_type* pBegin, const value_type* pEnd);

        // Iterators.
        iterator       begin();                 // Expanded in source code as: mpBegin
        const_iterator begin() const;           // Expanded in source code as: mpBegin
        iterator       end();                   // Expanded in source code as: mpEnd
        const_iterator end() const;             // Expanded in source code as: mpEnd

        reverse_iterator       rbegin();
        const_reverse_iterator rbegin() const;
        reverse_iterator       rend();
        const_reverse_iterator rend() const;

        // Size-related functionality
        bool      empty() const;                // Expanded in source code as: (mpBegin == mpEnd) or (mpBegin != mpEnd)
        size_type size() const;                 // Expanded in source code as: (size_type)(mpEnd - mpBegin)
        size_type length() const;               // Expanded in source code as: (size_type)(mpEnd - mpBegin)
        size_type max_size() const;             // Expanded in source code as: kMaxSize
        size_type capacity() const;             // Expanded in source code as: (size_type)((mpCapacity - mpBegin) - 1)
        void      resize(size_type n, value_type c);
        void      resize(size_type n);
        void      reserve(size_type = 0);
        void      set_capacity(size_type n = npos); // Revises the capacity to the user-specified value. Resizes the container to match the capacity if the requested capacity n is less than the current size. If n == npos then the capacity is reallocated (if necessary) such that capacity == size.
        void      force_size(size_type n);          // Unilaterally moves the string end position (mpEnd) to the given location. Useful for when the user writes into the string via some extenal means such as C strcpy or sprintf. This allows for more efficient use than using resize to achieve this.

        // Raw access
        const value_type* data() const;
        const value_type* c_str() const;

        // Element access
        reference       operator[](size_type n);
        const_reference operator[](size_type n) const;
        reference       at(size_type n);
        const_reference at(size_type n) const;
        reference       front();
        const_reference front() const;
        reference       back();
        const_reference back() const;

        // Append operations
        basic_string& operator+=(const basic_string& x);
        basic_string& operator+=(const value_type* p);
        basic_string& operator+=(value_type c);

        basic_string& append(const basic_string& x);
        basic_string& append(const basic_string& x, size_type position, size_type n);
        basic_string& append(const value_type* p, size_type n);
        basic_string& append(const value_type* p);
        basic_string& append(size_type n, value_type c);
        basic_string& append(const value_type* pBegin, const value_type* pEnd);

        basic_string& append_sprintf_va_list(const value_type* pFormat, va_list arguments);
        basic_string& append_sprintf(const value_type* pFormat, ...);

        void push_back(value_type c);
        void pop_back();

        // Insertion operations
        basic_string& insert(size_type position, const basic_string& x);
        basic_string& insert(size_type position, const basic_string& x, size_type beg, size_type n);
        basic_string& insert(size_type position, const value_type* p, size_type n);
        basic_string& insert(size_type position, const value_type* p);
        basic_string& insert(size_type position, size_type n, value_type c);
        iterator      insert(iterator p, value_type c);
        void          insert(iterator p, size_type n, value_type c);
        void          insert(iterator p, const value_type* pBegin, const value_type* pEnd);

        // Erase operations
        basic_string&    erase(size_type position = 0, size_type n = npos);
        iterator         erase(iterator p);
        iterator         erase(iterator pBegin, iterator pEnd);
        reverse_iterator erase(reverse_iterator position);
        reverse_iterator erase(reverse_iterator first, reverse_iterator last);
        void             clear();
        void             reset();                      // This is a unilateral reset to an initially empty state. No destructors are called, no deallocation occurs.

        //Replacement operations
        basic_string&  replace(size_type position, size_type n, const basic_string& x);
        basic_string&  replace(size_type pos1, size_type n1, const basic_string& x, size_type pos2, size_type n2);
        basic_string&  replace(size_type position, size_type n1, const value_type* p, size_type n2);
        basic_string&  replace(size_type position, size_type n1, const value_type* p);
        basic_string&  replace(size_type position, size_type n1, size_type n2, value_type c);
        basic_string&  replace(iterator first, iterator last, const basic_string& x);
        basic_string&  replace(iterator first, iterator last, const value_type* p, size_type n);
        basic_string&  replace(iterator first, iterator last, const value_type* p);
        basic_string&  replace(iterator first, iterator last, size_type n, value_type c);
        basic_string&  replace(iterator first, iterator last, const value_type* pBegin, const value_type* pEnd);
        size_type      copy(value_type* p, size_type n, size_type position = 0) const;

        // Find operations
        size_type find(const basic_string& x, size_type position = 0) const; 
        size_type find(const value_type* p, size_type position = 0) const;
        size_type find(const value_type* p, size_type position, size_type n) const;
        size_type find(value_type c, size_type position = 0) const;

        // Reverse find operations
        size_type rfind(const basic_string& x, size_type position = npos) const; 
        size_type rfind(const value_type* p, size_type position = npos) const;
        size_type rfind(const value_type* p, size_type position, size_type n) const;
        size_type rfind(value_type c, size_type position = npos) const;

        // Find first-of operations
        size_type find_first_of(const basic_string& x, size_type position = 0) const;
        size_type find_first_of(const value_type* p, size_type position = 0) const;
        size_type find_first_of(const value_type* p, size_type position, size_type n) const;
        size_type find_first_of(value_type c, size_type position = 0) const;

        // Find last-of operations
        size_type find_last_of(const basic_string& x, size_type position = npos) const;
        size_type find_last_of(const value_type* p, size_type position = npos) const;
        size_type find_last_of(const value_type* p, size_type position, size_type n) const;
        size_type find_last_of(value_type c, size_type position = npos) const;

        // Find first not-of operations
        size_type find_first_not_of(const basic_string& x, size_type position = 0) const;
        size_type find_first_not_of(const value_type* p, size_type position = 0) const;
        size_type find_first_not_of(const value_type* p, size_type position, size_type n) const;
        size_type find_first_not_of(value_type c, size_type position = 0) const;

        // Find last not-of operations
        size_type find_last_not_of(const basic_string& x,  size_type position = npos) const;
        size_type find_last_not_of(const value_type* p, size_type position = npos) const;
        size_type find_last_not_of(const value_type* p, size_type position, size_type n) const;
        size_type find_last_not_of(value_type c, size_type position = npos) const;

        // Substring functionality
        basic_string substr(size_type position = 0, size_type n = npos) const;

        // Comparison operations
        int        compare(const basic_string& x) const;
        int        compare(size_type pos1, size_type n1, const basic_string& x) const;
        int        compare(size_type pos1, size_type n1, const basic_string& x, size_type pos2, size_type n2) const;
        int        compare(const value_type* p) const;
        int        compare(size_type pos1, size_type n1, const value_type* p) const;
        int        compare(size_type pos1, size_type n1, const value_type* p, size_type n2) const;
        static int compare(const value_type* pBegin1, const value_type* pEnd1, const value_type* pBegin2, const value_type* pEnd2);

        // Case-insensitive comparison functions. Not part of C++ basic_string. Only ASCII-level locale functionality is supported. Thus this is not suitable for localization purposes.
        int        comparei(const basic_string& x) const;
        int        comparei(const value_type* p) const;
        static int comparei(const value_type* pBegin1, const value_type* pEnd1, const value_type* pBegin2, const value_type* pEnd2);

        // Misc functionality, not part of C++ basic_string.
        void            make_lower();
        void            make_upper();
        void            ltrim();
        void            rtrim();
        void            trim();
        basic_string    left(size_type n) const;
        basic_string    right(size_type n) const;
        basic_string&   sprintf_va_list(const value_type* pFormat, va_list arguments);
        basic_string&   sprintf(const value_type* pFormat, ...);

        bool validate() const;
        int  validate_iterator(const_iterator i) const;

    protected:
        // Helper functions for initialization/insertion operations.
        value_type* DoAllocate(size_type n);
        void        DoFree(value_type* p, size_type n);
        size_type   GetNewCapacity(size_type currentCapacity);

        void        AllocateSelf();
        void        AllocateSelf(size_type n);
        void        DeallocateSelf();
        iterator    InsertInternal(iterator p, value_type c);
        void        RangeInitialize(const value_type* pBegin, const value_type* pEnd);
        void        RangeInitialize(const value_type* pBegin);
        void        SizeInitialize(size_type n, value_type c);
        void        ThrowLengthException() const;
        void        ThrowRangeException() const;
        void        ThrowInvalidArgumentException() const;

        // Replacements for STL template functions.
        static const value_type* CharTypeStringFindEnd(const value_type* pBegin, const value_type* pEnd, value_type c);
        static const value_type* CharTypeStringRFind(const value_type* pRBegin, const value_type* pREnd, const value_type c);
        static const value_type* CharTypeStringSearch(const value_type* p1Begin, const value_type* p1End, const value_type* p2Begin, const value_type* p2End);
        static const value_type* CharTypeStringRSearch(const value_type* p1Begin, const value_type* p1End, const value_type* p2Begin, const value_type* p2End);
        static const value_type* CharTypeStringFindFirstOf(const value_type* p1Begin, const value_type* p1End, const value_type* p2Begin, const value_type* p2End);
        static const value_type* CharTypeStringRFindFirstOf(const value_type* p1RBegin, const value_type* p1REnd, const value_type* p2Begin, const value_type* p2End);
        static const value_type* CharTypeStringFindFirstNotOf(const value_type* p1Begin, const value_type* p1End, const value_type* p2Begin, const value_type* p2End);
        static const value_type* CharTypeStringRFindFirstNotOf(const value_type* p1RBegin, const value_type* p1REnd, const value_type* p2Begin, const value_type* p2End);

    }; // basic_string




    ///////////////////////////////////////////////////////////////////////////////
    // 'char traits' functionality
    //
    inline char8_t CharToLower(char8_t c)
        { return (char8_t)tolower((uint8_t)c); }

    inline char16_t CharToLower(char16_t c)
        { if((unsigned)c <= 0xff) return (char16_t)tolower((uint8_t)c); return c; }

    inline char32_t CharToLower(char32_t c)
        { if((unsigned)c <= 0xff) return (char32_t)tolower((uint8_t)c); return c; }



    inline char8_t CharToUpper(char8_t c)
        { return (char8_t)toupper((uint8_t)c); }

    inline char16_t CharToUpper(char16_t c)
        { if((unsigned)c <= 0xff) return (char16_t)toupper((uint8_t)c); return c; }

    inline char32_t CharToUpper(char32_t c)
        { if((unsigned)c <= 0xff) return (char32_t)toupper((uint8_t)c); return c; }



    template <typename T>
    int Compare(const T* p1, const T* p2, size_t n)
    {
        for(; n > 0; ++p1, ++p2, --n)
        {
            if(*p1 != *p2)
                return (*p1 < *p2) ? -1 : 1;
        }
        return 0;
    }

    inline int Compare(const char8_t* p1, const char8_t* p2, size_t n)
    {
        return memcmp(p1, p2, n);
    }

    template <typename T>
    inline int CompareI(const T* p1, const T* p2, size_t n)
    {
        for(; n > 0; ++p1, ++p2, --n)
        {
            const T c1 = CharToLower(*p1);
            const T c2 = CharToLower(*p2);

            if(c1 != c2)
                return (c1 < c2) ? -1 : 1;
        }
        return 0;
    }


    inline const char8_t* Find(const char8_t* p, char8_t c, size_t n)
    {
        return (const char8_t*)memchr(p, c, n);
    }

    inline const char16_t* Find(const char16_t* p, char16_t c, size_t n)
    {
        for(; n > 0; --n, ++p)
        {
            if(*p == c)
                return p;
        }

        return NULL;
    }

    inline const char32_t* Find(const char32_t* p, char32_t c, size_t n)
    {
        for(; n > 0; --n, ++p)
        {
            if(*p == c)
                return p;
        }

        return NULL;
    }


    inline size_t CharStrlen(const char8_t* p)
    {
        #ifdef _MSC_VER // VC++ can implement an instrinsic here.
            return strlen(p);
        #else
            const char8_t* pCurrent = p;
            while(*pCurrent)
                ++pCurrent;
            return (size_t)(pCurrent - p);
        #endif
    }

    inline size_t CharStrlen(const char16_t* p)
    {
        const char16_t* pCurrent = p;
        while(*pCurrent)
            ++pCurrent;
        return (size_t)(pCurrent - p);
    }

    inline size_t CharStrlen(const char32_t* p)
    {
        const char32_t* pCurrent = p;
        while(*pCurrent)
            ++pCurrent;
        return (size_t)(pCurrent - p);
    }


    template <typename T>
    inline T* CharStringUninitializedCopy(const T* pSource, const T* pSourceEnd, T* pDestination)
    {
        memmove(pDestination, pSource, (size_t)(pSourceEnd - pSource) * sizeof(T));
        return pDestination + (pSourceEnd - pSource);
    }




    inline char8_t* CharStringUninitializedFillN(char8_t* pDestination, size_t n, const char8_t c)
    {
        if(n) // Some compilers (e.g. GCC 4.3+) generate a warning (which can't be disabled) if you call memset with a size of 0.
            memset(pDestination, (uint8_t)c, (size_t)n);
        return pDestination + n;
    }

    inline char16_t* CharStringUninitializedFillN(char16_t* pDestination, size_t n, const char16_t c)
    {
        char16_t* pDest16          = pDestination;
        const char16_t* const pEnd = pDestination + n;
        while(pDest16 < pEnd)
            *pDest16++ = c;
        return pDestination + n;
    }

    inline char32_t* CharStringUninitializedFillN(char32_t* pDestination, size_t n, const char32_t c)
    {
        char32_t* pDest32          = pDestination;
        const char32_t* const pEnd = pDestination + n;
        while(pDest32 < pEnd)
            *pDest32++ = c;
        return pDestination + n;
    }



    inline char8_t* CharTypeAssignN(char8_t* pDestination, size_t n, char8_t c)
    {
        if(n) // Some compilers (e.g. GCC 4.3+) generate a warning (which can't be disabled) if you call memset with a size of 0.
            return (char8_t*)memset(pDestination, c, (size_t)n);
        return pDestination;
    }

    inline char16_t* CharTypeAssignN(char16_t* pDestination, size_t n, char16_t c)
    {
        char16_t* pDest16          = pDestination;
        const char16_t* const pEnd = pDestination + n;
        while(pDest16 < pEnd)
            *pDest16++ = c;
        return pDestination;
    }

    inline char32_t* CharTypeAssignN(char32_t* pDestination, size_t n, char32_t c)
    {
        char32_t* pDest32          = pDestination;
        const char32_t* const pEnd = pDestination + n;
        while(pDest32 < pEnd)
            *pDest32++ = c;
        return pDestination;
    }



    ///////////////////////////////////////////////////////////////////////////////
    // basic_string
    ///////////////////////////////////////////////////////////////////////////////

    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>::basic_string()
        : mpBegin(NULL),
          mpEnd(NULL),
          mpCapacity(NULL),
          mAllocator(EASTL_BASIC_STRING_DEFAULT_NAME)
    {
        AllocateSelf();
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>::basic_string(const allocator_type& allocator)
        : mpBegin(NULL),
          mpEnd(NULL),
          mpCapacity(NULL),
          mAllocator(allocator)
    {
        AllocateSelf();
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>::basic_string(const this_type& x)
        : mpBegin(NULL),
          mpEnd(NULL),
          mpCapacity(NULL),
          mAllocator(x.mAllocator)
    {
        RangeInitialize(x.mpBegin, x.mpEnd);
    }

#ifdef EA_COMPILER_HAS_MOVE_SEMANTICS
    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>::basic_string(this_type&& x)
        : mpBegin(NULL),
          mpEnd(NULL),
          mpCapacity(NULL),
          mAllocator(x.mAllocator)
    {
        swap(x);
    }
#endif


    template <typename T, typename Allocator>
    basic_string<T, Allocator>::basic_string(const this_type& x, size_type position, size_type n) 
        : mpBegin(NULL),
          mpEnd(NULL),
          mpCapacity(NULL),
          mAllocator(x.mAllocator)
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY(position > (size_type)(x.mpEnd - x.mpBegin)))
            {
                ThrowRangeException();
                AllocateSelf();
            }
            else
                RangeInitialize(x.mpBegin + position, x.mpBegin + position + eastl::min_alt(n, (size_type)(x.mpEnd - x.mpBegin) - position));
        #else
            RangeInitialize(x.mpBegin + position, x.mpBegin + position + eastl::min_alt(n, (size_type)(x.mpEnd - x.mpBegin) - position));
        #endif
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>::basic_string(const value_type* p, size_type n, const allocator_type& allocator) 
        : mpBegin(NULL),
          mpEnd(NULL),
          mpCapacity(NULL),
          mAllocator(allocator)
    {
        RangeInitialize(p, p + n);
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>::basic_string(const value_type* p, const allocator_type& allocator)
        : mpBegin(NULL),
          mpEnd(NULL),
          mpCapacity(NULL),
          mAllocator(allocator)
    {
        RangeInitialize(p);
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>::basic_string(size_type n, value_type c, const allocator_type& allocator)
        : mpBegin(NULL),
          mpEnd(NULL),
          mpCapacity(NULL),
          mAllocator(allocator)
    {
        SizeInitialize(n, c);
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>::basic_string(const value_type* pBegin, const value_type* pEnd, const allocator_type& allocator)
        : mpBegin(NULL),
          mpEnd(NULL),
          mpCapacity(NULL),
          mAllocator(allocator)
    {
        RangeInitialize(pBegin, pEnd);
    }


    // CtorDoNotInitialize exists so that we can create a version that allocates but doesn't 
    // initialize but also doesn't collide with any other constructor declaration.
    template <typename T, typename Allocator>
    basic_string<T, Allocator>::basic_string(CtorDoNotInitialize /*unused*/, size_type n, const allocator_type& allocator)
        : mpBegin(NULL),
          mpEnd(NULL),
          mpCapacity(NULL),
          mAllocator(allocator)
    {
        // Note that we do not call SizeInitialize here.
        AllocateSelf(n + 1); // '+1' so that we have room for the terminating 0.
        *mpEnd = 0;
    }


    // CtorSprintf exists so that we can create a version that does a variable argument
    // sprintf but also doesn't collide with any other constructor declaration.
    template <typename T, typename Allocator>
    basic_string<T, Allocator>::basic_string(CtorSprintf /*unused*/, const value_type* pFormat, ...)
        : mpBegin(NULL),
          mpEnd(NULL),
          mpCapacity(NULL),
          mAllocator()
    {
        const size_type n = (size_type)CharStrlen(pFormat) + 1; // We'll need at least this much. '+1' so that we have room for the terminating 0.
        AllocateSelf(n); 

        va_list arguments;
        va_start(arguments, pFormat);
        append_sprintf_va_list(pFormat, arguments);
        va_end(arguments);
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>::~basic_string()
    {
        DeallocateSelf();
    }


    template <typename T, typename Allocator>
    inline const typename basic_string<T, Allocator>::allocator_type&
    basic_string<T, Allocator>::get_allocator() const
    {
        return mAllocator;
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::allocator_type&
    basic_string<T, Allocator>::get_allocator()
    {
        return mAllocator;
    }


    template <typename T, typename Allocator>
    inline void basic_string<T, Allocator>::set_allocator(const allocator_type& allocator)
    {
        mAllocator = allocator;
    }


    template <typename T, typename Allocator>
    inline const typename basic_string<T, Allocator>::value_type*
    basic_string<T, Allocator>::data()  const
    {
        return mpBegin;
    }


    template <typename T, typename Allocator>
    inline const typename basic_string<T, Allocator>::value_type*
    basic_string<T, Allocator>::c_str() const
    {
        return mpBegin;
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::iterator
    basic_string<T, Allocator>::begin()
    {
        return mpBegin;
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::iterator
    basic_string<T, Allocator>::end()
    {
        return mpEnd;
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::const_iterator
    basic_string<T, Allocator>::begin() const
    {
        return mpBegin;
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::const_iterator
    basic_string<T, Allocator>::end() const
    {
        return mpEnd;
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::reverse_iterator
    basic_string<T, Allocator>::rbegin()
    {
        return reverse_iterator(mpEnd);
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::reverse_iterator
    basic_string<T, Allocator>::rend()
    {
        return reverse_iterator(mpBegin);
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::const_reverse_iterator
    basic_string<T, Allocator>::rbegin() const
    {
        return const_reverse_iterator(mpEnd);
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::const_reverse_iterator
    basic_string<T, Allocator>::rend() const
    {
        return const_reverse_iterator(mpBegin);
    }


    template <typename T, typename Allocator>
    inline bool basic_string<T, Allocator>::empty() const
    {
        return (mpBegin == mpEnd);
    }     


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::size() const
    {
        return (size_type)(mpEnd - mpBegin);
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::length() const
    {
        return (size_type)(mpEnd - mpBegin);
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::max_size() const
    {
        return kMaxSize;
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::capacity() const
    {
        return (size_type)((mpCapacity - mpBegin) - 1); // '-1' because we pretend that we didn't allocate memory for the terminating 0.
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::const_reference
    basic_string<T, Allocator>::operator[](size_type n) const
    {
        #if EASTL_ASSERT_ENABLED // We allow the user to reference the trailing 0 char without asserting. Perhaps we shouldn't.
            if(EASTL_UNLIKELY(n > (static_cast<size_type>(mpEnd - mpBegin))))
                EASTL_FAIL_MSG("basic_string::operator[] -- out of range");
        #endif

        return mpBegin[n]; // Sometimes done as *(mpBegin + n)
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::reference
    basic_string<T, Allocator>::operator[](size_type n)
    {
        #if EASTL_ASSERT_ENABLED // We allow the user to reference the trailing 0 char without asserting. Perhaps we shouldn't.
            if(EASTL_UNLIKELY(n > (static_cast<size_type>(mpEnd - mpBegin))))
                EASTL_FAIL_MSG("basic_string::operator[] -- out of range");
        #endif

        return mpBegin[n]; // Sometimes done as *(mpBegin + n)
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::this_type& basic_string<T, Allocator>::operator=(const basic_string<T, Allocator>& x)
    {
        if(&x != this)
        {
            #if EASTL_ALLOCATOR_COPY_ENABLED
                mAllocator = x.mAllocator;
            #endif

            assign(x.mpBegin, x.mpEnd);
        }
        return *this;
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::this_type& basic_string<T, Allocator>::operator=(const value_type* p)
    {
        return assign(p, p + CharStrlen(p));
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::this_type& basic_string<T, Allocator>::operator=(value_type c)
    {
        return assign((size_type)1, c);
    }


    template <typename T, typename Allocator>
    void basic_string<T, Allocator>::resize(size_type n, value_type c)
    {
        const size_type s = (size_type)(mpEnd - mpBegin);

        if(n < s)
            erase(mpBegin + n, mpEnd);
        else if(n > s)
            append(n - s, c);
    }


    template <typename T, typename Allocator>
    void basic_string<T, Allocator>::resize(size_type n)
    {
        // C++ basic_string specifies that resize(n) is equivalent to resize(n, value_type()). 
        // For built-in types, value_type() is the same as zero (value_type(0)).
        // We can improve the efficiency (especially for long strings) of this 
        // string class by resizing without assigning to anything.
        
        const size_type s = (size_type)(mpEnd - mpBegin);

        if(n < s)
            erase(mpBegin + n, mpEnd);
        else if(n > s)
        {
            #if EASTL_STRING_OPT_CHAR_INIT
                append(n - s, value_type());
            #else 
                append(n - s);
            #endif
        }
    }


    template <typename T, typename Allocator>
    void basic_string<T, Allocator>::reserve(size_type n)
    {
        #if EASTL_STRING_OPT_LENGTH_ERRORS
            if(EASTL_UNLIKELY(n > kMaxSize))
                ThrowLengthException();
        #endif

        // The C++ standard for basic_string doesn't specify if we should or shouldn't 
        // downsize the container. The standard is overly vague in its description of reserve:
        //    The member function reserve() is a directive that informs a 
        //    basic_string object of a planned change in size, so that it 
        //    can manage the storage allocation accordingly.
        // We will act like the vector container and preserve the contents of 
        // the container and only reallocate if increasing the size. The user 
        // can use the set_capacity function to reduce the capacity.

        n = eastl::max_alt(n, (size_type)(mpEnd - mpBegin)); // Calculate the new capacity, which needs to be >= container size.

        if(n >= (size_type)(mpCapacity - mpBegin))  // If there is something to do... // We use >= because mpCapacity accounts for the trailing zero.
            set_capacity(n);
    }


    template <typename T, typename Allocator>
    inline void basic_string<T, Allocator>::set_capacity(size_type n)
    {
        if(n == npos) // If the user wants to set the capacity to equal the current size... // '-1' because we pretend that we didn't allocate memory for the terminating 0.
            n = (size_type)(mpEnd - mpBegin);
        else if(n < (size_type)(mpEnd - mpBegin))
            mpEnd = mpBegin + n;

        if(n != (size_type)((mpCapacity - mpBegin) - 1)) // If there is any capacity change...
        {
            if(n)
            {
                pointer pNewBegin = DoAllocate(n + 1); // We need the + 1 to accomodate the trailing 0.
                pointer pNewEnd   = pNewBegin;

                pNewEnd = CharStringUninitializedCopy(mpBegin, mpEnd, pNewBegin);
               *pNewEnd = 0;

                DeallocateSelf();
                mpBegin    = pNewBegin;
                mpEnd      = pNewEnd;
                mpCapacity = pNewBegin + (n + 1);
            }
            else
            {
                DeallocateSelf();
                AllocateSelf();
            }
        }
    }


    template <typename T, typename Allocator>
    inline void basic_string<T, Allocator>::force_size(size_type n)
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY(n >= (size_type)(mpCapacity - mpBegin)))
                ThrowRangeException();
        #elif EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(n >= (size_type)(mpCapacity - mpBegin)))
                EASTL_FAIL_MSG("basic_string::force_size -- out of range");
        #endif

        mpEnd = mpBegin + n;
    }


    template <typename T, typename Allocator>
    inline void basic_string<T, Allocator>::clear()
    {
        if(mpBegin != mpEnd)
        {
           *mpBegin = value_type(0);
            mpEnd   = mpBegin;
        }
    } 


    template <typename T, typename Allocator>
    inline void basic_string<T, Allocator>::reset()
    {
        // The reset function is a special extension function which unilaterally 
        // resets the container to an empty state without freeing the memory of 
        // the contained objects. This is useful for very quickly tearing down a 
        // container built into scratch memory.
        AllocateSelf();
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::const_reference
    basic_string<T, Allocator>::at(size_type n) const
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY(n >= (size_type)(mpEnd - mpBegin)))
                ThrowRangeException();
        #elif EASTL_ASSERT_ENABLED                  // We assert if the user references the trailing 0 char.
            if(EASTL_UNLIKELY(n >= (size_type)(mpEnd - mpBegin)))
                EASTL_FAIL_MSG("basic_string::at -- out of range");
        #endif

        return mpBegin[n];
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::reference
    basic_string<T, Allocator>::at(size_type n)
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY(n >= (size_type)(mpEnd - mpBegin)))
                ThrowRangeException();
        #elif EASTL_ASSERT_ENABLED                  // We assert if the user references the trailing 0 char.
            if(EASTL_UNLIKELY(n >= (size_type)(mpEnd - mpBegin)))
                EASTL_FAIL_MSG("basic_string::at -- out of range");
        #endif

        return mpBegin[n];
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::reference
    basic_string<T, Allocator>::front()
    {
        #if EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
            // We allow the user to reference the trailing 0 char without asserting.
        #elif EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(mpEnd <= mpBegin)) // We assert if the user references the trailing 0 char.
                EASTL_FAIL_MSG("basic_string::front -- empty string");
        #endif

        return *mpBegin;
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::const_reference
    basic_string<T, Allocator>::front() const
    {
        #if EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
            // We allow the user to reference the trailing 0 char without asserting.
        #elif EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(mpEnd <= mpBegin)) // We assert if the user references the trailing 0 char.
                EASTL_FAIL_MSG("basic_string::front -- empty string");
        #endif

        return *mpBegin;
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::reference
    basic_string<T, Allocator>::back()
    {
        #if EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
            // We allow the user to reference the trailing 0 char without asserting.
        #elif EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(mpEnd <= mpBegin)) // We assert if the user references the trailing 0 char.
                EASTL_FAIL_MSG("basic_string::back -- empty string");
        #endif

        return *(mpEnd - 1);
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::const_reference
    basic_string<T, Allocator>::back() const
    {
        #if EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
            // We allow the user to reference the trailing 0 char without asserting.
        #elif EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(mpEnd <= mpBegin)) // We assert if the user references the trailing 0 char.
                EASTL_FAIL_MSG("basic_string::back -- empty string");
        #endif

        return *(mpEnd - 1);
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>& basic_string<T, Allocator>::operator+=(const basic_string<T, Allocator>& x)
    {
        return append(x);
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>& basic_string<T, Allocator>::operator+=(const value_type* p)
    {
        return append(p);
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>& basic_string<T, Allocator>::operator+=(value_type c)
    {
        push_back(c);
        return *this;
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>& basic_string<T, Allocator>::append(const basic_string<T, Allocator>& x)
    {
        return append(x.mpBegin, x.mpEnd);
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>& basic_string<T, Allocator>::append(const basic_string<T, Allocator>& x, size_type position, size_type n)
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY(position > (size_type)(x.mpEnd - x.mpBegin)))
                ThrowRangeException();
        #endif

        return append(x.mpBegin + position, x.mpBegin + position + eastl::min_alt(n, (size_type)(x.mpEnd - x.mpBegin) - position));
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>& basic_string<T, Allocator>::append(const value_type* p, size_type n)
    {
        return append(p, p + n);
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>& basic_string<T, Allocator>::append(const value_type* p)
    {
        return append(p, p + CharStrlen(p));
    }


    template <typename T, typename Allocator>
    basic_string<T, Allocator>& basic_string<T, Allocator>::append(size_type n, value_type c)
    {
        const size_type s = (size_type)(mpEnd - mpBegin);

        #if EASTL_STRING_OPT_LENGTH_ERRORS
            if(EASTL_UNLIKELY((n > kMaxSize) || (s > (kMaxSize - n))))
                ThrowLengthException();
        #endif

        const size_type nCapacity = (size_type)((mpCapacity - mpBegin) - 1);

        if((s + n) > nCapacity)
            reserve(eastl::max_alt((size_type)GetNewCapacity(nCapacity), (size_type)(s + n)));

        if(n > 0)
        {
            CharStringUninitializedFillN(mpEnd + 1, n - 1, c);
           *mpEnd  = c;
            mpEnd += n;
           *mpEnd  = 0;
        }

        return *this;
    }


    template <typename T, typename Allocator>
    basic_string<T, Allocator>& basic_string<T, Allocator>::append(const value_type* pBegin, const value_type* pEnd)
    {
        if(pBegin != pEnd)
        {
            const size_type nOldSize = (size_type)(mpEnd - mpBegin);
            const size_type n        = (size_type)(pEnd - pBegin);

            #if EASTL_STRING_OPT_LENGTH_ERRORS
                if(EASTL_UNLIKELY(((size_t)n > kMaxSize) || (nOldSize > (kMaxSize - n))))
                    ThrowLengthException();
            #endif

            const size_type nCapacity = (size_type)((mpCapacity - mpBegin) - 1);

            if((nOldSize + n) > nCapacity)
            {
                const size_type nLength = eastl::max_alt((size_type)GetNewCapacity(nCapacity), (size_type)(nOldSize + n)) + 1; // + 1 to accomodate the trailing 0.

                pointer pNewBegin = DoAllocate(nLength);
                pointer pNewEnd   = pNewBegin;

                pNewEnd = CharStringUninitializedCopy(mpBegin, mpEnd, pNewBegin);
                pNewEnd = CharStringUninitializedCopy(pBegin,  pEnd,  pNewEnd);
               *pNewEnd = 0;

                DeallocateSelf();
                mpBegin    = pNewBegin;
                mpEnd      = pNewEnd;
                mpCapacity = pNewBegin + nLength; 
            }
            else
            {
                const value_type* pTemp = pBegin;
                ++pTemp;
                CharStringUninitializedCopy(pTemp, pEnd, mpEnd + 1);
                mpEnd[n] = 0;
               *mpEnd    = *pBegin;
                mpEnd   += n;
            }
        }

        return *this; 
    }


    template <typename T, typename Allocator>
    basic_string<T, Allocator>& basic_string<T, Allocator>::append_sprintf_va_list(const value_type* pFormat, va_list arguments)
    {
        // From unofficial C89 extension documentation:
        // The vsnprintf returns the number of characters written into the array,
        // not counting the terminating null character, or a negative value
        // if count or more characters are requested to be generated.
        // An error can occur while converting a value for output.

        // From the C99 standard:
        // The vsnprintf function returns the number of characters that would have
        // been written had n been sufficiently large, not counting the terminating
        // null character, or a negative value if an encoding error occurred.
        // Thus, the null-terminated output has been completely written if and only
        // if the returned value is nonnegative and less than n.
        size_type nInitialSize = (size_type)(mpEnd - mpBegin);
        int       nReturnValue;

        #if EASTL_VA_COPY_ENABLED
            va_list argumentsSaved;
            va_copy(argumentsSaved, arguments);
        #endif

        if(mpBegin == GetEmptyString(value_type())) // We need to do this because non-standard vsnprintf implementations will otherwise overwrite gEmptyString with a non-zero char.
            nReturnValue = eastl::Vsnprintf(mpEnd, 0, pFormat, arguments);
        else
            nReturnValue = eastl::Vsnprintf(mpEnd, (size_t)(mpCapacity - mpEnd), pFormat, arguments);

        if(nReturnValue >= (int)(mpCapacity - mpEnd))  // If there wasn't enough capacity...
        {
            // In this case we definitely have C99 Vsnprintf behaviour.
            #if EASTL_VA_COPY_ENABLED
                va_copy(arguments, argumentsSaved);
            #endif
            resize(nInitialSize + nReturnValue);
            nReturnValue = eastl::Vsnprintf(mpBegin + nInitialSize, (size_t)(nReturnValue + 1), pFormat, arguments); // '+1' because vsnprintf wants to know the size of the buffer including the terminating zero.
        }
        else if(nReturnValue < 0) // If vsnprintf is non-C99-standard (e.g. it is VC++ _vsnprintf)...
        {
            // In this case we either have C89 extension behaviour or C99 behaviour.
            size_type n = eastl::max_alt((size_type)(EASTL_STRING_INITIAL_CAPACITY - 1), (size_type)(size() * 2)); // '-1' because the resize call below will add one for NULL terminator and we want to keep allocations on fixed block sizes.
    
            for(; (nReturnValue < 0) && (n < 1000000); n *= 2)
            {
                #if EASTL_VA_COPY_ENABLED
                    va_copy(arguments, argumentsSaved);
                #endif
                resize(n);

                const size_t nCapacity = (size_t)((n + 1) - nInitialSize);
                nReturnValue = eastl::Vsnprintf(mpBegin + nInitialSize, nCapacity, pFormat, arguments); // '+1' because vsnprintf wants to know the size of the buffer including the terminating zero.

                if(nReturnValue == (int)(unsigned)nCapacity)
                {
                    resize(++n);
                    nReturnValue = eastl::Vsnprintf(mpBegin + nInitialSize, nCapacity + 1, pFormat, arguments);
                }
            }
        }
     
        if(nReturnValue >= 0)
            mpEnd = mpBegin + nInitialSize + nReturnValue; // We are guaranteed from the above logic that mpEnd <= mpCapacity.
    
        return *this;
    }

    template <typename T, typename Allocator>
    basic_string<T, Allocator>& basic_string<T, Allocator>::append_sprintf(const value_type* pFormat, ...)
    {
        va_list arguments;
        va_start(arguments, pFormat);
        append_sprintf_va_list(pFormat, arguments);
        va_end(arguments);
        
        return *this;
    }


    template <typename T, typename Allocator>
    inline void basic_string<T, Allocator>::push_back(value_type c)
    {
        if((mpEnd + 1) == mpCapacity) // If we are out of space... (note that we test for + 1 because we have a trailing 0)
            reserve(eastl::max_alt(GetNewCapacity((size_type)((mpCapacity - mpBegin) - 1)), (size_type)(mpEnd - mpBegin) + 1));
        *mpEnd++ = c;
        *mpEnd   = 0;
    }


    template <typename T, typename Allocator>
    inline void basic_string<T, Allocator>::pop_back()
    {
        #if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(mpEnd <= mpBegin))
                EASTL_FAIL_MSG("basic_string::pop_back -- empty string");
        #endif

        mpEnd[-1] = value_type(0);
        --mpEnd;
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>& basic_string<T, Allocator>::assign(const basic_string<T, Allocator>& x)
    {
        return assign(x.mpBegin, x.mpEnd);
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>& basic_string<T, Allocator>::assign(const basic_string<T, Allocator>& x, size_type position, size_type n)
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY(position > (size_type)(x.mpEnd - x.mpBegin)))
                ThrowRangeException();
        #endif

        return assign(x.mpBegin + position, x.mpBegin + position + eastl::min_alt(n, (size_type)(x.mpEnd - x.mpBegin) - position));
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>& basic_string<T, Allocator>::assign(const value_type* p, size_type n)
    {
        return assign(p, p + n);
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>& basic_string<T, Allocator>::assign(const value_type* p)
    {
        return assign(p, p + CharStrlen(p));
    }


    template <typename T, typename Allocator>
    basic_string<T, Allocator>& basic_string<T, Allocator>::assign(size_type n, value_type c)
    {
        if(n <= (size_type)(mpEnd - mpBegin))
        {
            CharTypeAssignN(mpBegin, n, c);
            erase(mpBegin + n, mpEnd);
        }
        else
        {
            CharTypeAssignN(mpBegin, (size_type)(mpEnd - mpBegin), c);
            append(n - (size_type)(mpEnd - mpBegin), c);
        }
        return *this;
    }


    template <typename T, typename Allocator>
    basic_string<T, Allocator>& basic_string<T, Allocator>::assign(const value_type* pBegin, const value_type* pEnd)
    {
        const ptrdiff_t n = pEnd - pBegin;
        if(static_cast<size_type>(n) <= (size_type)(mpEnd - mpBegin))
        {
            memmove(mpBegin, pBegin, (size_t)n * sizeof(value_type));
            erase(mpBegin + n, mpEnd);
        }
        else
        {
            memmove(mpBegin, pBegin, (size_t)(mpEnd - mpBegin) * sizeof(value_type));
            append(pBegin + (size_type)(mpEnd - mpBegin), pEnd);
        }
        return *this;
    }


    template <typename T, typename Allocator>
    basic_string<T, Allocator>& basic_string<T, Allocator>::insert(size_type position, const basic_string<T, Allocator>& x)
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY(position > (size_type)(mpEnd - mpBegin)))
                ThrowRangeException();
        #endif

        #if EASTL_STRING_OPT_LENGTH_ERRORS
            if(EASTL_UNLIKELY((size_type)(mpEnd - mpBegin) > (kMaxSize - (size_type)(x.mpEnd - x.mpBegin))))
                ThrowLengthException();
        #endif

        insert(mpBegin + position, x.mpBegin, x.mpEnd);
        return *this;
    }


    template <typename T, typename Allocator>
    basic_string<T, Allocator>& basic_string<T, Allocator>::insert(size_type position, const basic_string<T, Allocator>& x, size_type beg, size_type n)
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY((position > (size_type)(mpEnd - mpBegin)) || (beg > (size_type)(x.mpEnd - x.mpBegin))))
                ThrowRangeException();
        #endif

        size_type nLength = eastl::min_alt(n, (size_type)(x.mpEnd - x.mpBegin) - beg);

        #if EASTL_STRING_OPT_LENGTH_ERRORS
            if(EASTL_UNLIKELY((size_type)(mpEnd - mpBegin) > (kMaxSize - nLength)))
                ThrowLengthException();
        #endif

        insert(mpBegin + position, x.mpBegin + beg, x.mpBegin + beg + nLength);
        return *this;
    }


    template <typename T, typename Allocator>
    basic_string<T, Allocator>& basic_string<T, Allocator>::insert(size_type position, const value_type* p, size_type n)
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY(position > (size_type)(mpEnd - mpBegin)))
                ThrowRangeException();
        #endif

        #if EASTL_STRING_OPT_LENGTH_ERRORS
            if(EASTL_UNLIKELY((size_type)(mpEnd - mpBegin) > (kMaxSize - n)))
                ThrowLengthException();
        #endif

        insert(mpBegin + position, p, p + n);
        return *this;
    }


    template <typename T, typename Allocator>
    basic_string<T, Allocator>& basic_string<T, Allocator>::insert(size_type position, const value_type* p)
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY(position > (size_type)(mpEnd - mpBegin)))
                ThrowRangeException();
        #endif

        size_type nLength = (size_type)CharStrlen(p);

        #if EASTL_STRING_OPT_LENGTH_ERRORS
            if(EASTL_UNLIKELY((size_type)(mpEnd - mpBegin) > (kMaxSize - nLength)))
                ThrowLengthException();
        #endif

        insert(mpBegin + position, p, p + nLength);
        return *this;
    }


    template <typename T, typename Allocator>
    basic_string<T, Allocator>& basic_string<T, Allocator>::insert(size_type position, size_type n, value_type c)
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY(position > (size_type)(mpEnd - mpBegin)))
                ThrowRangeException();
        #endif

        #if EASTL_STRING_OPT_LENGTH_ERRORS
            if(EASTL_UNLIKELY((size_type)(mpEnd - mpBegin) > (kMaxSize - n)))
                ThrowLengthException();
        #endif

        insert(mpBegin + position, n, c);
        return *this;
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::iterator
    basic_string<T, Allocator>::insert(iterator p, value_type c)
    {
        if(p == mpEnd)
        {
            push_back(c);
            return mpEnd - 1;
        }
        return InsertInternal(p, c);
    }


    template <typename T, typename Allocator>
    void basic_string<T, Allocator>::insert(iterator p, size_type n, value_type c)
    {
        #if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY((p < mpBegin) || (p > mpEnd)))
                EASTL_FAIL_MSG("basic_string::insert -- invalid position");
        #endif

        if(n) // If there is anything to insert...
        {
            if(size_type(mpCapacity - mpEnd) >= (n + 1)) // If we have enough capacity...
            {
                const size_type nElementsAfter = (size_type)(mpEnd - p);
                iterator pOldEnd = mpEnd;

                if(nElementsAfter >= n) // If there's enough space for the new chars between the insert position and the end...
                {
                    CharStringUninitializedCopy((mpEnd - n) + 1, mpEnd + 1, mpEnd + 1);
                    mpEnd += n;
                    memmove(p + n, p, (size_t)((nElementsAfter - n) + 1) * sizeof(value_type));
                    CharTypeAssignN(p, n, c);
                }
                else
                {
                    CharStringUninitializedFillN(mpEnd + 1, n - nElementsAfter - 1, c);
                    mpEnd += n - nElementsAfter;

                    #if EASTL_EXCEPTIONS_ENABLED
                        try
                        {
                    #endif
                            CharStringUninitializedCopy(p, pOldEnd + 1, mpEnd);
                            mpEnd += nElementsAfter;
                    #if EASTL_EXCEPTIONS_ENABLED
                        }
                        catch(...)
                        {
                            mpEnd = pOldEnd;
                            throw;
                        }
                    #endif

                    CharTypeAssignN(p, nElementsAfter + 1, c);
                }
            }
            else
            {
                const size_type nOldSize = (size_type)(mpEnd - mpBegin);
                const size_type nOldCap  = (size_type)((mpCapacity - mpBegin) - 1);
                const size_type nLength  = eastl::max_alt((size_type)GetNewCapacity(nOldCap), (size_type)(nOldSize + n)) + 1; // + 1 to accomodate the trailing 0.

                iterator pNewBegin = DoAllocate(nLength);
                iterator pNewEnd   = pNewBegin;

                pNewEnd = CharStringUninitializedCopy(mpBegin, p, pNewBegin);
                pNewEnd = CharStringUninitializedFillN(pNewEnd, n, c);
                pNewEnd = CharStringUninitializedCopy(p, mpEnd, pNewEnd);
               *pNewEnd = 0;

                DeallocateSelf();
                mpBegin    = pNewBegin;
                mpEnd      = pNewEnd;
                mpCapacity = pNewBegin + nLength;     
            }
        }
    }


    template <typename T, typename Allocator>
    void basic_string<T, Allocator>::insert(iterator p, const value_type* pBegin, const value_type* pEnd)
    {
        #if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY((p < mpBegin) || (p > mpEnd)))
                EASTL_FAIL_MSG("basic_string::insert -- invalid position");
        #endif

        const size_type n = (size_type)(pEnd - pBegin);

        if(n)
        {
            const bool bCapacityIsSufficient = ((mpCapacity - mpEnd) >= (difference_type)(n + 1));
            const bool bSourceIsFromSelf     = ((pEnd >= mpBegin) && (pBegin <= mpEnd));

            // If bSourceIsFromSelf is true, then we reallocate. This is because we are 
            // inserting ourself into ourself and thus both the source and destination 
            // be modified, making it rather tricky to attempt to do in place. The simplest
            // resolution is to reallocate. To consider: there may be a way to implement this
            // whereby we don't need to reallocate or can often avoid reallocating.
            if(bCapacityIsSufficient && !bSourceIsFromSelf)
            {
                const ptrdiff_t nElementsAfter = (mpEnd - p);
                iterator        pOldEnd        = mpEnd;

                if(nElementsAfter >= (ptrdiff_t)n) // If the newly inserted characters entirely fit within the size of the original string...
                {
                    memmove(mpEnd + 1, mpEnd - n + 1, (size_t)n * sizeof(value_type));
                    mpEnd += n;
                    memmove(p + n, p, (size_t)((nElementsAfter - n) + 1) * sizeof(value_type));
                    memmove(p, pBegin, (size_t)(pEnd - pBegin) * sizeof(value_type));
                }
                else
                {
                    const value_type* const pMid = pBegin + (nElementsAfter + 1);

                    memmove(mpEnd + 1, pMid, (size_t)(pEnd - pMid) * sizeof(value_type));
                    mpEnd += n - nElementsAfter;

                    #if EASTL_EXCEPTIONS_ENABLED
                        try
                        {
                    #endif
                            memmove(mpEnd, p, (size_t)(pOldEnd - p + 1) * sizeof(value_type));
                            mpEnd += nElementsAfter;
                    #if EASTL_EXCEPTIONS_ENABLED
                        }
                        catch(...)
                        {
                            mpEnd = pOldEnd;
                            throw;
                        }
                    #endif

                    memmove(p, pBegin, (size_t)(pMid - pBegin) * sizeof(value_type));
                }
            }
            else // Else we need to reallocate to implement this.
            {
                const size_type nOldSize = (size_type)(mpEnd - mpBegin);
                const size_type nOldCap  = (size_type)((mpCapacity - mpBegin) - 1);
                size_type nLength;

                if(bCapacityIsSufficient) // If bCapacityIsSufficient is true, then bSourceIsFromSelf must be false.
                    nLength = nOldSize + n + 1; // + 1 to accomodate the trailing 0.
                else
                    nLength = eastl::max_alt((size_type)GetNewCapacity(nOldCap), (size_type)(nOldSize + n)) + 1; // + 1 to accomodate the trailing 0.

                pointer pNewBegin = DoAllocate(nLength);
                pointer pNewEnd   = pNewBegin;

                pNewEnd = CharStringUninitializedCopy(mpBegin, p,     pNewBegin);
                pNewEnd = CharStringUninitializedCopy(pBegin,  pEnd,  pNewEnd);
                pNewEnd = CharStringUninitializedCopy(p,       mpEnd, pNewEnd);
               *pNewEnd = 0;

                DeallocateSelf();
                mpBegin    = pNewBegin;
                mpEnd      = pNewEnd;
                mpCapacity = pNewBegin + nLength; 
            }
        }
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>& basic_string<T, Allocator>::erase(size_type position, size_type n)
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY(position > (size_type)(mpEnd - mpBegin)))
                ThrowRangeException();
        #endif

        #if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(position > (size_type)(mpEnd - mpBegin)))
                EASTL_FAIL_MSG("basic_string::erase -- invalid position");
        #endif

        erase(mpBegin + position, mpBegin + position + eastl::min_alt(n, (size_type)(mpEnd - mpBegin) - position));
        return *this;
    }  


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::iterator
    basic_string<T, Allocator>::erase(iterator p)
    {
        #if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY((p < mpBegin) || (p >= mpEnd)))
                EASTL_FAIL_MSG("basic_string::erase -- invalid position");
        #endif

        memmove(p, p + 1, (size_t)(mpEnd - p) * sizeof(value_type));
        --mpEnd;
        return p;
    }


    template <typename T, typename Allocator>
    typename basic_string<T, Allocator>::iterator
    basic_string<T, Allocator>::erase(iterator pBegin, iterator pEnd)
    {
        #if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY((pBegin < mpBegin) || (pBegin > mpEnd) || (pEnd < mpBegin) || (pEnd > mpEnd) || (pEnd < pBegin)))
                EASTL_FAIL_MSG("basic_string::erase -- invalid position");
        #endif

        if(pBegin != pEnd)
        {
            memmove(pBegin, pEnd, (size_t)((mpEnd - pEnd) + 1) * sizeof(value_type));
            const iterator pNewEnd = (mpEnd - (pEnd - pBegin));
            mpEnd = pNewEnd;
        }
        return pBegin;
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::reverse_iterator
    basic_string<T, Allocator>::erase(reverse_iterator position)
    {
        return reverse_iterator(erase((++position).base()));
    }


    template <typename T, typename Allocator>
    typename basic_string<T, Allocator>::reverse_iterator
    basic_string<T, Allocator>::erase(reverse_iterator first, reverse_iterator last)
    {
        return reverse_iterator(erase((++last).base(), (++first).base()));
    }


    template <typename T, typename Allocator>
    basic_string<T, Allocator>& basic_string<T, Allocator>::replace(size_type position, size_type n, const basic_string<T, Allocator>& x)
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY(position > (size_type)(mpEnd - mpBegin)))
                ThrowRangeException();
        #endif

        const size_type nLength = eastl::min_alt(n, (size_type)(mpEnd - mpBegin) - position);

        #if EASTL_STRING_OPT_LENGTH_ERRORS
            if(EASTL_UNLIKELY(((size_type)(mpEnd - mpBegin) - nLength) >= (kMaxSize - (size_type)(x.mpEnd - x.mpBegin))))
                ThrowLengthException();
        #endif

        return replace(mpBegin + position, mpBegin + position + nLength, x.mpBegin, x.mpEnd);
    }


    template <typename T, typename Allocator>
    basic_string<T, Allocator>& basic_string<T, Allocator>::replace(size_type pos1, size_type n1, const basic_string<T, Allocator>& x, size_type pos2, size_type n2)
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY((pos1 > (size_type)(mpEnd - mpBegin)) || (pos2 > (size_type)(x.mpEnd - x.mpBegin))))
                ThrowRangeException();
        #endif

        const size_type nLength1 = eastl::min_alt(n1, (size_type)(  mpEnd -   mpBegin) - pos1);
        const size_type nLength2 = eastl::min_alt(n2, (size_type)(x.mpEnd - x.mpBegin) - pos2);

        #if EASTL_STRING_OPT_LENGTH_ERRORS
            if(EASTL_UNLIKELY(((size_type)(mpEnd - mpBegin) - nLength1) >= (kMaxSize - nLength2)))
                ThrowLengthException();
        #endif

        return replace(mpBegin + pos1, mpBegin + pos1 + nLength1, x.mpBegin + pos2, x.mpBegin + pos2 + nLength2);
    }


    template <typename T, typename Allocator>
    basic_string<T, Allocator>& basic_string<T, Allocator>::replace(size_type position, size_type n1, const value_type* p, size_type n2)
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY(position > (size_type)(mpEnd - mpBegin)))
                ThrowRangeException();
        #endif

        const size_type nLength = eastl::min_alt(n1, (size_type)(mpEnd - mpBegin) - position);

        #if EASTL_STRING_OPT_LENGTH_ERRORS
            if(EASTL_UNLIKELY((n2 > kMaxSize) || (((size_type)(mpEnd - mpBegin) - nLength) >= (kMaxSize - n2))))
                ThrowLengthException();
        #endif

        return replace(mpBegin + position, mpBegin + position + nLength, p, p + n2);
    }


    template <typename T, typename Allocator>
    basic_string<T, Allocator>& basic_string<T, Allocator>::replace(size_type position, size_type n1, const value_type* p)
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY(position > (size_type)(mpEnd - mpBegin)))
                ThrowRangeException();
        #endif

        const size_type nLength = eastl::min_alt(n1, (size_type)(mpEnd - mpBegin) - position);

        #if EASTL_STRING_OPT_LENGTH_ERRORS
            const size_type n2 = (size_type)CharStrlen(p);
            if(EASTL_UNLIKELY((n2 > kMaxSize) || (((size_type)(mpEnd - mpBegin) - nLength) >= (kMaxSize - n2))))
                ThrowLengthException();
        #endif

        return replace(mpBegin + position, mpBegin + position + nLength, p, p + CharStrlen(p));
    }


    template <typename T, typename Allocator>
    basic_string<T, Allocator>& basic_string<T, Allocator>::replace(size_type position, size_type n1, size_type n2, value_type c)
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY(position > (size_type)(mpEnd - mpBegin)))
                ThrowRangeException();
        #endif

        const size_type nLength = eastl::min_alt(n1, (size_type)(mpEnd - mpBegin) - position);

        #if EASTL_STRING_OPT_LENGTH_ERRORS
            if(EASTL_UNLIKELY((n2 > kMaxSize) || ((size_type)(mpEnd - mpBegin) - nLength) >= (kMaxSize - n2)))
                ThrowLengthException();
        #endif

        return replace(mpBegin + position, mpBegin + position + nLength, n2, c);
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>& basic_string<T, Allocator>::replace(iterator pBegin, iterator pEnd, const basic_string<T, Allocator>& x)
    {
        return replace(pBegin, pEnd, x.mpBegin, x.mpEnd);
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>& basic_string<T, Allocator>::replace(iterator pBegin, iterator pEnd, const value_type* p, size_type n)
    {
        return replace(pBegin, pEnd, p, p + n);
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>& basic_string<T, Allocator>::replace(iterator pBegin, iterator pEnd, const value_type* p)
    {
        return replace(pBegin, pEnd, p, p + CharStrlen(p));
    }


    template <typename T, typename Allocator>
    basic_string<T, Allocator>& basic_string<T, Allocator>::replace(iterator pBegin, iterator pEnd, size_type n, value_type c)
    {
        #if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY((pBegin < mpBegin) || (pBegin > mpEnd) || (pEnd < mpBegin) || (pEnd > mpEnd) || (pEnd < pBegin)))
                EASTL_FAIL_MSG("basic_string::replace -- invalid position");
        #endif

        const size_type nLength = static_cast<size_type>(pEnd - pBegin);

        if(nLength >= n)
        {
            CharTypeAssignN(pBegin, n, c);
            erase(pBegin + n, pEnd);
        }
        else
        {
            CharTypeAssignN(pBegin, nLength, c);
            insert(pEnd, n - nLength, c);
        }
        return *this;
    }


    template <typename T, typename Allocator>
    basic_string<T, Allocator>& basic_string<T, Allocator>::replace(iterator pBegin1, iterator pEnd1, const value_type* pBegin2, const value_type* pEnd2)
    {
        #if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY((pBegin1 < mpBegin) || (pBegin1 > mpEnd) || (pEnd1 < mpBegin) || (pEnd1 > mpEnd) || (pEnd1 < pBegin1)))
                EASTL_FAIL_MSG("basic_string::replace -- invalid position");
        #endif

        const size_type nLength1 = (size_type)(pEnd1 - pBegin1);
        const size_type nLength2 = (size_type)(pEnd2 - pBegin2);

        if(nLength1 >= nLength2) // If we have a non-expanding operation...
        {
            if((pBegin2 > pEnd1) || (pEnd2 <= pBegin1))  // If we have a non-overlapping operation...
                memcpy(pBegin1, pBegin2, (size_t)(pEnd2 - pBegin2) * sizeof(value_type));
            else
                memmove(pBegin1, pBegin2, (size_t)(pEnd2 - pBegin2) * sizeof(value_type));
            erase(pBegin1 + nLength2, pEnd1);
        }
        else // Else we are expanding.
        {
            if((pBegin2 > pEnd1) || (pEnd2 <= pBegin1)) // If we have a non-overlapping operation...
            {
                const value_type* const pMid2 = pBegin2 + nLength1;

                if((pEnd2 <= pBegin1) || (pBegin2 > pEnd1))
                    memcpy(pBegin1, pBegin2, (size_t)(pMid2 - pBegin2) * sizeof(value_type));
                else
                    memmove(pBegin1, pBegin2, (size_t)(pMid2 - pBegin2) * sizeof(value_type));
                insert(pEnd1, pMid2, pEnd2);
            }
            else // else we have an overlapping operation.
            {
                // I can't think of any easy way of doing this without allocating temporary memory.
                const size_type nOldSize     = (size_type)(mpEnd - mpBegin);
                const size_type nOldCap      = (size_type)((mpCapacity - mpBegin) - 1);
                const size_type nNewCapacity = eastl::max_alt((size_type)GetNewCapacity(nOldCap), (size_type)(nOldSize + (nLength2 - nLength1))) + 1; // + 1 to accomodate the trailing 0.

                pointer pNewBegin = DoAllocate(nNewCapacity);
                pointer pNewEnd   = pNewBegin;

                pNewEnd = CharStringUninitializedCopy(mpBegin, pBegin1, pNewBegin);
                pNewEnd = CharStringUninitializedCopy(pBegin2, pEnd2,   pNewEnd);
                pNewEnd = CharStringUninitializedCopy(pEnd1,   mpEnd,   pNewEnd);
               *pNewEnd = 0;

                DeallocateSelf();
                mpBegin    = pNewBegin;
                mpEnd      = pNewEnd;
                mpCapacity = pNewBegin + nNewCapacity; 
            }
        }
        return *this;
    }


    template <typename T, typename Allocator>
    typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::copy(value_type* p, size_type n, size_type position) const
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY(position > (size_type)(mpEnd - mpBegin)))
                ThrowRangeException();
        #endif

        // It is not clear from the C++ standard if 'p' destination pointer is allowed to 
        // refer to memory from within the string itself. We assume so and use memmove 
        // instead of memcpy until we find otherwise.
        const size_type nLength = eastl::min_alt(n, (size_type)(mpEnd - mpBegin) - position);
        memmove(p, mpBegin + position, (size_t)nLength * sizeof(value_type));
        return nLength;
    }


    template <typename T, typename Allocator>
    void basic_string<T, Allocator>::swap(basic_string<T, Allocator>& x)
    {
        if(mAllocator == x.mAllocator) // If allocators are equivalent...
        {
            // We leave mAllocator as-is.
            eastl::swap(mpBegin,     x.mpBegin);
            eastl::swap(mpEnd,       x.mpEnd);
            eastl::swap(mpCapacity,  x.mpCapacity);
        }
        else // else swap the contents.
        {
            const this_type temp(*this); // Can't call eastl::swap because that would
            *this = x;                   // itself call this member swap function.
            x     = temp;
        }
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::find(const basic_string<T, Allocator>& x, size_type position) const
    {
        return find(x.mpBegin, position, (size_type)(x.mpEnd - x.mpBegin));
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::find(const value_type* p, size_type position) const
    {
        return find(p, position, (size_type)CharStrlen(p));
    }


    #if defined(EA_PLATFORM_XENON) // If XBox 360...
       
        template <typename T, typename Allocator>
        typename basic_string<T, Allocator>::size_type
        basic_string<T, Allocator>::find(const value_type* p, size_type position, size_type n) const
        {
            const size_type nLength = (size_type)(mpEnd - mpBegin);

            if(n || (position > nLength))
            {
                if(position < nLength)
                {
                    size_type nRemain = nLength - position;

                    if(n <= nRemain)
                    {
                        nRemain -= (n - 1);

                        for(const value_type* p1, *p2 = mpBegin + position;
                            (p1 = Find(p2, *p, nRemain)) != 0;
                            nRemain -= (p1 - p2) + 1, p2 = (p1 + 1))
                        {
                            if(Compare(p1, p, n) == 0)
                                return (size_type)(p1 - mpBegin);
                        }
                    }
                }

                return npos;
            }

            return position;
        }
    #else
        template <typename T, typename Allocator>
        typename basic_string<T, Allocator>::size_type
        basic_string<T, Allocator>::find(const value_type* p, size_type position, size_type n) const
        {
            // It is not clear what the requirements are for position, but since the C++ standard
            // appears to be silent it is assumed for now that position can be any value.
            //#if EASTL_ASSERT_ENABLED
            //    if(EASTL_UNLIKELY(position > (size_type)(mpEnd - mpBegin)))
            //        EASTL_FAIL_MSG("basic_string::find -- invalid position");
            //#endif

            if(EASTL_LIKELY((position + n) <= (size_type)(mpEnd - mpBegin))) // If the range is valid...
            {
                const value_type* const pTemp = eastl::search(mpBegin + position, mpEnd, p, p + n);

                if((pTemp != mpEnd) || (n == 0))
                    return (size_type)(pTemp - mpBegin);
            }
            return npos;
        }
    #endif


    template <typename T, typename Allocator>
    typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::find(value_type c, size_type position) const
    {
        // It is not clear what the requirements are for position, but since the C++ standard
        // appears to be silent it is assumed for now that position can be any value.
        //#if EASTL_ASSERT_ENABLED
        //    if(EASTL_UNLIKELY(position > (size_type)(mpEnd - mpBegin)))
        //        EASTL_FAIL_MSG("basic_string::find -- invalid position");
        //#endif

        if(EASTL_LIKELY(position < (size_type)(mpEnd - mpBegin))) // If the position is valid...
        {
            const const_iterator pResult = eastl::find(mpBegin + position, mpEnd, c);

            if(pResult != mpEnd)
                return (size_type)(pResult - mpBegin);
        }
        return npos;
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::rfind(const basic_string<T, Allocator>& x, size_type position) const
    {
        return rfind(x.mpBegin, position, (size_type)(x.mpEnd - x.mpBegin));
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::rfind(const value_type* p, size_type position) const
    {
        return rfind(p, position, (size_type)CharStrlen(p));
    }


    template <typename T, typename Allocator>
    typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::rfind(const value_type* p, size_type position, size_type n) const
    {
        // Disabled because it's not clear what values are valid for position. 
        // It is documented that npos is a valid value, though. We return npos and 
        // don't crash if postion is any invalid value.
        //#if EASTL_ASSERT_ENABLED
        //    if(EASTL_UNLIKELY((position != npos) && (position > (size_type)(mpEnd - mpBegin))))
        //        EASTL_FAIL_MSG("basic_string::rfind -- invalid position");
        //#endif

        // Note that a search for a zero length string starting at position = end() returns end() and not npos.
        // Note by Paul Pedriana: I am not sure how this should behave in the case of n == 0 and position > size. 
        // The standard seems to suggest that rfind doesn't act exactly the same as find in that input position 
        // can be > size and the return value can still be other than npos. Thus, if n == 0 then you can 
        // never return npos, unlike the case with find.
        const size_type nLength = (size_type)(mpEnd - mpBegin);

        if(EASTL_LIKELY(n <= nLength))
        {
            if(EASTL_LIKELY(n))
            {
                const const_iterator pEnd    = mpBegin + eastl::min_alt(nLength - n, position) + n;
                const const_iterator pResult = CharTypeStringRSearch(mpBegin, pEnd, p, p + n);

                if(pResult != pEnd)
                    return (size_type)(pResult - mpBegin);
            }
            else
                return eastl::min_alt(nLength, position);
        }
        return npos;
    }


    template <typename T, typename Allocator>
    typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::rfind(value_type c, size_type position) const
    {
        // If n is zero or position is >= size, we return npos.
        const size_type nLength = (size_type)(mpEnd - mpBegin);

        if(EASTL_LIKELY(nLength))
        {
            const value_type* const pEnd    = mpBegin + eastl::min_alt(nLength - 1, position) + 1;
            const value_type* const pResult = CharTypeStringRFind(pEnd, mpBegin, c);

            if(pResult != mpBegin)
                return (size_type)((pResult - 1) - mpBegin);
        }
        return npos;
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::find_first_of(const basic_string<T, Allocator>& x, size_type position) const
    {
        return find_first_of(x.mpBegin, position, (size_type)(x.mpEnd - x.mpBegin));
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::size_type 
    basic_string<T, Allocator>::find_first_of(const value_type* p, size_type position) const
    {
        return find_first_of(p, position, (size_type)CharStrlen(p));
    }


    #if defined(EA_PLATFORM_XENON) // If XBox 360...
        
        template <typename T, typename Allocator>
        typename basic_string<T, Allocator>::size_type
        basic_string<T, Allocator>::find_first_of(const value_type* p, size_type position, size_type n) const
        {
            // If position is >= size, we return npos.
            if(n && (position < (size_type)(mpEnd - mpBegin)))
            {
                for(const value_type* p1 = (mpBegin + position); p1 < mpEnd; ++p1)
                {
                    if(Find(p, *p1, n) != 0)
                        return (size_type)(p1 - mpBegin);
                }
            }
            return npos;
        }
    #else
        template <typename T, typename Allocator>
        typename basic_string<T, Allocator>::size_type
        basic_string<T, Allocator>::find_first_of(const value_type* p, size_type position, size_type n) const
        {
            // If position is >= size, we return npos.
            if(EASTL_LIKELY((position < (size_type)(mpEnd - mpBegin))))
            {
                const value_type* const pBegin = mpBegin + position;
                const const_iterator pResult   = CharTypeStringFindFirstOf(pBegin, mpEnd, p, p + n);

                if(pResult != mpEnd)
                    return (size_type)(pResult - mpBegin);
            }
            return npos;
        }
    #endif


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::find_first_of(value_type c, size_type position) const
    {
        return find(c, position);
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::find_last_of(const basic_string<T, Allocator>& x, size_type position) const
    {
        return find_last_of(x.mpBegin, position, (size_type)(x.mpEnd - x.mpBegin));
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::find_last_of(const value_type* p, size_type position) const
    {
        return find_last_of(p, position, (size_type)CharStrlen(p));
    }


    #if defined(EA_PLATFORM_XENON) // If XBox 360...
       
        template <typename T, typename Allocator>
        typename basic_string<T, Allocator>::size_type
        basic_string<T, Allocator>::find_last_of(const value_type* p, size_type position, size_type n) const
        {
            // If n is zero or position is >= size, we return npos.
            const size_type nLength = (size_type)(mpEnd - mpBegin);

            if(n && nLength)
            {
                const value_type* p1;

                if(position < nLength)
                    p1 = mpBegin + position;
                else
                    p1 = mpEnd - 1;

                for(;;)
                {
                    if(Find(p, *p1, n))
                        return (size_type)(p1 - mpBegin);

                    if(p1-- == mpBegin)
                        break;
                }
            }

            return npos;
        }
    #else
        template <typename T, typename Allocator>
        typename basic_string<T, Allocator>::size_type
        basic_string<T, Allocator>::find_last_of(const value_type* p, size_type position, size_type n) const
        {
            // If n is zero or position is >= size, we return npos.
            const size_type nLength = (size_type)(mpEnd - mpBegin);

            if(EASTL_LIKELY(nLength))
            {
                const value_type* const pEnd    = mpBegin + eastl::min_alt(nLength - 1, position) + 1;
                const value_type* const pResult = CharTypeStringRFindFirstOf(pEnd, mpBegin, p, p + n);

                if(pResult != mpBegin)
                    return (size_type)((pResult - 1) - mpBegin);
            }
            return npos;
        }
    #endif


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::find_last_of(value_type c, size_type position) const
    {
        return rfind(c, position);
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::find_first_not_of(const basic_string<T, Allocator>& x, size_type position) const
    {
        return find_first_not_of(x.mpBegin, position, (size_type)(x.mpEnd - x.mpBegin));
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::find_first_not_of(const value_type* p, size_type position) const
    {
        return find_first_not_of(p, position, (size_type)CharStrlen(p));
    }


    template <typename T, typename Allocator>
    typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::find_first_not_of(const value_type* p, size_type position, size_type n) const
    {
        if(EASTL_LIKELY(position <= (size_type)(mpEnd - mpBegin)))
        {
            const const_iterator pResult = CharTypeStringFindFirstNotOf(mpBegin + position, mpEnd, p, p + n);

            if(pResult != mpEnd)
                return (size_type)(pResult - mpBegin);
        }
        return npos;
    }


    template <typename T, typename Allocator>
    typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::find_first_not_of(value_type c, size_type position) const
    {
        if(EASTL_LIKELY(position <= (size_type)(mpEnd - mpBegin)))
        {
            // Todo: Possibly make a specialized version of CharTypeStringFindFirstNotOf(pBegin, pEnd, c).
            const const_iterator pResult = CharTypeStringFindFirstNotOf(mpBegin + position, mpEnd, &c, &c + 1);

            if(pResult != mpEnd)
                return (size_type)(pResult - mpBegin);
        }
        return npos;
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::find_last_not_of(const basic_string<T, Allocator>& x, size_type position) const
    {
        return find_last_not_of(x.mpBegin, position, (size_type)(x.mpEnd - x.mpBegin));
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::find_last_not_of(const value_type* p, size_type position) const
    {
        return find_last_not_of(p, position, (size_type)CharStrlen(p));
    }


    template <typename T, typename Allocator>
    typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::find_last_not_of(const value_type* p, size_type position, size_type n) const
    {
        const size_type nLength = (size_type)(mpEnd - mpBegin);

        if(EASTL_LIKELY(nLength))
        {
            const value_type* const pEnd    = mpBegin + eastl::min_alt(nLength - 1, position) + 1;
            const value_type* const pResult = CharTypeStringRFindFirstNotOf(pEnd, mpBegin, p, p + n);

            if(pResult != mpBegin)
                return (size_type)((pResult - 1) - mpBegin);
        }
        return npos;
    }


    template <typename T, typename Allocator>
    typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::find_last_not_of(value_type c, size_type position) const
    {
        const size_type nLength = (size_type)(mpEnd - mpBegin);

        if(EASTL_LIKELY(nLength))
        {
            // Todo: Possibly make a specialized version of CharTypeStringRFindFirstNotOf(pBegin, pEnd, c).
            const value_type* const pEnd    = mpBegin + eastl::min_alt(nLength - 1, position) + 1;
            const value_type* const pResult = CharTypeStringRFindFirstNotOf(pEnd, mpBegin, &c, &c + 1);

            if(pResult != mpBegin)
                return (size_type)((pResult - 1) - mpBegin);
        }
        return npos;
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator> basic_string<T, Allocator>::substr(size_type position, size_type n) const
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY(position > (size_type)(mpEnd - mpBegin)))
                ThrowRangeException();
        #elif EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(position > (size_type)(mpEnd - mpBegin)))
                EASTL_FAIL_MSG("basic_string::substr -- invalid position");
        #endif

        return basic_string(mpBegin + position, mpBegin + position + eastl::min_alt(n, (size_type)(mpEnd - mpBegin) - position), mAllocator);
    }


    template <typename T, typename Allocator>
    inline int basic_string<T, Allocator>::compare(const basic_string<T, Allocator>& x) const
    {
        return compare(mpBegin, mpEnd, x.mpBegin, x.mpEnd);
    }


    template <typename T, typename Allocator>
    inline int basic_string<T, Allocator>::compare(size_type pos1, size_type n1, const basic_string<T, Allocator>& x) const
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY(pos1 > (size_type)(mpEnd - mpBegin)))
                ThrowRangeException();
        #endif

        return compare(mpBegin + pos1, 
                       mpBegin + pos1 + eastl::min_alt(n1, (size_type)(mpEnd - mpBegin) - pos1),
                       x.mpBegin,
                       x.mpEnd);
    }


    template <typename T, typename Allocator>
    inline int basic_string<T, Allocator>::compare(size_type pos1, size_type n1, const basic_string<T, Allocator>& x, size_type pos2, size_type n2) const
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY((pos1 > (size_type)(mpEnd - mpBegin)) || (pos2 > (size_type)(x.mpEnd - x.mpBegin))))
                ThrowRangeException();
        #endif

        return compare(mpBegin + pos1, 
                       mpBegin + pos1 + eastl::min_alt(n1, (size_type)(mpEnd - mpBegin) - pos1),
                       x.mpBegin + pos2, 
                       x.mpBegin + pos2 + eastl::min_alt(n2, (size_type)(mpEnd - mpBegin) - pos2));
    }


    template <typename T, typename Allocator>
    inline int basic_string<T, Allocator>::compare(const value_type* p) const
    {
        return compare(mpBegin, mpEnd, p, p + CharStrlen(p));
    }


    template <typename T, typename Allocator>
    inline int basic_string<T, Allocator>::compare(size_type pos1, size_type n1, const value_type* p) const
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY(pos1 > (size_type)(mpEnd - mpBegin)))
                ThrowRangeException();
        #endif

        return compare(mpBegin + pos1, 
                       mpBegin + pos1 + eastl::min_alt(n1, (size_type)(mpEnd - mpBegin) - pos1),
                       p,
                       p + CharStrlen(p));
    }


    template <typename T, typename Allocator>
    inline int basic_string<T, Allocator>::compare(size_type pos1, size_type n1, const value_type* p, size_type n2) const
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(EASTL_UNLIKELY(pos1 > (size_type)(mpEnd - mpBegin)))
                ThrowRangeException();
        #endif

        return compare(mpBegin + pos1, 
                       mpBegin + pos1 + eastl::min_alt(n1, (size_type)(mpEnd - mpBegin) - pos1),
                       p,
                       p + n2);
    }


    // make_lower
    // This is a very simple ASCII-only case conversion function
    // Anything more complicated should use a more powerful separate library.
    template <typename T, typename Allocator>
    inline void basic_string<T, Allocator>::make_lower()
    {
        for(pointer p = mpBegin; p < mpEnd; ++p)
            *p = (value_type)CharToLower(*p);
    }


    // make_upper
    // This is a very simple ASCII-only case conversion function
    // Anything more complicated should use a more powerful separate library.
    template <typename T, typename Allocator>
    inline void basic_string<T, Allocator>::make_upper()
    {
        for(pointer p = mpBegin; p < mpEnd; ++p)
            *p = (value_type)CharToUpper(*p);
    }


    template <typename T, typename Allocator>
    inline void basic_string<T, Allocator>::ltrim()
    {
        const value_type array[] = { ' ', '\t', 0 }; // This is a pretty simplistic view of whitespace.
        erase(0, find_first_not_of(array));
    }


    template <typename T, typename Allocator>
    inline void basic_string<T, Allocator>::rtrim()
    {
        const value_type array[] = { ' ', '\t', 0 }; // This is a pretty simplistic view of whitespace.
        erase(find_last_not_of(array) + 1);
    }


    template <typename T, typename Allocator>
    inline void basic_string<T, Allocator>::trim()
    {
        ltrim();
        rtrim();
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator> basic_string<T, Allocator>::left(size_type n) const
    {
        const size_type nLength = length();
        if(n < nLength)
            return substr(0, n);
        return *this;
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator> basic_string<T, Allocator>::right(size_type n) const
    {
        const size_type nLength = length();
        if(n < nLength)
            return substr(nLength - n, n);
        return *this;
    }


    template <typename T, typename Allocator>
    inline basic_string<T, Allocator>& basic_string<T, Allocator>::sprintf(const value_type* pFormat, ...)
    {
        va_list arguments;
        va_start(arguments, pFormat);
        mpEnd = mpBegin; // Fast truncate to zero length.
        append_sprintf_va_list(pFormat, arguments);
        va_end(arguments);

        return *this;
    }


    template <typename T, typename Allocator>
    basic_string<T, Allocator>& basic_string<T, Allocator>::sprintf_va_list(const value_type* pFormat, va_list arguments)
    {
        mpEnd = mpBegin; // Fast truncate to zero length.

        return append_sprintf_va_list(pFormat, arguments);
    }


    template <typename T, typename Allocator>
    int basic_string<T, Allocator>::compare(const value_type* pBegin1, const value_type* pEnd1,
                                            const value_type* pBegin2, const value_type* pEnd2)
    {
        const ptrdiff_t n1   = pEnd1 - pBegin1;
        const ptrdiff_t n2   = pEnd2 - pBegin2;
        const ptrdiff_t nMin = eastl::min_alt(n1, n2);
        const int       cmp  = Compare(pBegin1, pBegin2, (size_t)nMin);

        return (cmp != 0 ? cmp : (n1 < n2 ? -1 : (n1 > n2 ? 1 : 0)));
    }


    template <typename T, typename Allocator>
    int basic_string<T, Allocator>::comparei(const value_type* pBegin1, const value_type* pEnd1, 
                                             const value_type* pBegin2, const value_type* pEnd2)
    {
        const ptrdiff_t n1   = pEnd1 - pBegin1;
        const ptrdiff_t n2   = pEnd2 - pBegin2;
        const ptrdiff_t nMin = eastl::min_alt(n1, n2);
        const int       cmp  = CompareI(pBegin1, pBegin2, (size_t)nMin);

        return (cmp != 0 ? cmp : (n1 < n2 ? -1 : (n1 > n2 ? 1 : 0)));
    }


    template <typename T, typename Allocator>
    inline int basic_string<T, Allocator>::comparei(const basic_string<T, Allocator>& x) const
    {
        return comparei(mpBegin, mpEnd, x.mpBegin, x.mpEnd);
    }


    template <typename T, typename Allocator>
    inline int basic_string<T, Allocator>::comparei(const value_type* p) const
    {
        return comparei(mpBegin, mpEnd, p, p + CharStrlen(p));
    }


    template <typename T, typename Allocator>
    typename basic_string<T, Allocator>::iterator
    basic_string<T, Allocator>::InsertInternal(iterator p, value_type c)
    {
        iterator pNewPosition = p;

        if((mpEnd + 1) < mpCapacity)
        {
            *(mpEnd + 1) = 0;
            memmove(p + 1, p, (size_t)(mpEnd - p) * sizeof(value_type));
            *p = c;
            ++mpEnd;
        }
        else
        {
            const size_type nOldSize = (size_type)(mpEnd - mpBegin);
            const size_type nOldCap  = (size_type)((mpCapacity - mpBegin) - 1);
            const size_type nLength  = eastl::max_alt((size_type)GetNewCapacity(nOldCap), (size_type)(nOldSize + 1)) + 1; // The second + 1 is to accomodate the trailing 0.

            iterator pNewBegin = DoAllocate(nLength);
            iterator pNewEnd   = pNewBegin;

            pNewPosition = CharStringUninitializedCopy(mpBegin, p, pNewBegin);
           *pNewPosition = c;

            pNewEnd = pNewPosition + 1;
            pNewEnd = CharStringUninitializedCopy(p, mpEnd, pNewEnd);
           *pNewEnd = 0;

            DeallocateSelf();
            mpBegin    = pNewBegin;
            mpEnd      = pNewEnd;
            mpCapacity = pNewBegin + nLength;
        }
        return pNewPosition;
    }


    template <typename T, typename Allocator>
    void basic_string<T, Allocator>::SizeInitialize(size_type n, value_type c)
    {
        AllocateSelf((size_type)(n + 1)); // '+1' so that we have room for the terminating 0.

        mpEnd = CharStringUninitializedFillN(mpBegin, n, c);
       *mpEnd = 0;
    }


    template <typename T, typename Allocator>
    void basic_string<T, Allocator>::RangeInitialize(const value_type* pBegin, const value_type* pEnd)
    {
        const size_type n = (size_type)(pEnd - pBegin);

        #if EASTL_STRING_OPT_ARGUMENT_ERRORS
            if(EASTL_UNLIKELY(!pBegin && (n != 0)))
                ThrowInvalidArgumentException();
        #endif

        AllocateSelf((size_type)(n + 1)); // '+1' so that we have room for the terminating 0.

        mpEnd = CharStringUninitializedCopy(pBegin, pEnd, mpBegin);
       *mpEnd = 0;
    }


    template <typename T, typename Allocator>
    inline void basic_string<T, Allocator>::RangeInitialize(const value_type* pBegin)
    {
        #if EASTL_STRING_OPT_ARGUMENT_ERRORS
            if(EASTL_UNLIKELY(!pBegin))
                ThrowInvalidArgumentException();
        #endif

        RangeInitialize(pBegin, pBegin + CharStrlen(pBegin));
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::value_type*
    basic_string<T, Allocator>::DoAllocate(size_type n)
    {
        EASTL_ASSERT(n > 1); // We want n > 1 because n == 1 is reserved for empty capacity and usage of gEmptyString.
        return (value_type*)EASTLAlloc(mAllocator, n * sizeof(value_type));
    }


    template <typename T, typename Allocator>
    inline void basic_string<T, Allocator>::DoFree(value_type* p, size_type n)
    {
        if(p)
            EASTLFree(mAllocator, p, n * sizeof(value_type));
    }


    template <typename T, typename Allocator>
    inline typename basic_string<T, Allocator>::size_type
    basic_string<T, Allocator>::GetNewCapacity(size_type currentCapacity) // This needs to return a value of at least currentCapacity and at least 1.
    {
        return (currentCapacity > EASTL_STRING_INITIAL_CAPACITY) ? (2 * currentCapacity) : EASTL_STRING_INITIAL_CAPACITY;
    }


    template <typename T, typename Allocator>
    inline void basic_string<T, Allocator>::AllocateSelf()
    {
        EASTL_ASSERT(gEmptyString.mUint32 == 0);
        mpBegin     = const_cast<value_type*>(GetEmptyString(value_type()));  // In const_cast-int this, we promise not to modify it.
        mpEnd       = mpBegin;
        mpCapacity  = mpBegin + 1; // When we are using gEmptyString, mpCapacity is always mpEnd + 1. This is an important distinguising characteristic.
    }


    template <typename T, typename Allocator>
    void basic_string<T, Allocator>::AllocateSelf(size_type n)
    {
        #if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(n >= 0x40000000))
                EASTL_FAIL_MSG("basic_string::AllocateSelf -- improbably large request.");
        #endif

        #if EASTL_STRING_OPT_LENGTH_ERRORS
            if(EASTL_UNLIKELY(n > kMaxSize))
                ThrowLengthException();
        #endif

        if(n > 1)
        {
            mpBegin    = DoAllocate(n);
            mpEnd      = mpBegin;
            mpCapacity = mpBegin + n;
        }
        else
            AllocateSelf();
    }


    template <typename T, typename Allocator>
    inline void basic_string<T, Allocator>::DeallocateSelf()
    {
        // Note that we compare mpCapacity to mpEnd instead of comparing 
        // mpBegin to &gEmptyString. This is important because we may have
        // a case whereby one library passes a string to another library to 
        // deallocate and the two libraries have idependent versions of gEmptyString.
        if((mpCapacity - mpBegin) > 1) // If we are not using gEmptyString as our memory...
            DoFree(mpBegin, (size_type)(mpCapacity - mpBegin));
    }


    template <typename T, typename Allocator>
    inline void basic_string<T, Allocator>::ThrowLengthException() const
    {
        #if EASTL_EXCEPTIONS_ENABLED
            throw std::length_error("basic_string -- length_error");
        #elif EASTL_ASSERT_ENABLED
            EASTL_FAIL_MSG("basic_string -- length_error");
        #endif
    }


    template <typename T, typename Allocator>
    inline void basic_string<T, Allocator>::ThrowRangeException() const
    {
        #if EASTL_EXCEPTIONS_ENABLED
            throw std::out_of_range("basic_string -- out of range");
        #elif EASTL_ASSERT_ENABLED
            EASTL_FAIL_MSG("basic_string -- out of range");
        #endif
    }


    template <typename T, typename Allocator>
    inline void basic_string<T, Allocator>::ThrowInvalidArgumentException() const
    {
        #if EASTL_EXCEPTIONS_ENABLED
            throw std::invalid_argument("basic_string -- invalid argument");
        #elif EASTL_ASSERT_ENABLED
            EASTL_FAIL_MSG("basic_string -- invalid argument");
        #endif
    }


    // CharTypeStringFindEnd
    // Specialized char version of STL find() from back function.
    // Not the same as RFind because search range is specified as forward iterators.
    template <typename T, typename Allocator>
    const typename basic_string<T, Allocator>::value_type*
    basic_string<T, Allocator>::CharTypeStringFindEnd(const value_type* pBegin, const value_type* pEnd, value_type c)
    {
        const value_type* pTemp = pEnd;
        while(--pTemp >= pBegin)
        {
            if(*pTemp == c)
                return pTemp;
        }

        return pEnd;
    }


    // CharTypeStringRFind
    // Specialized value_type version of STL find() function in reverse.
    template <typename T, typename Allocator>
    const typename basic_string<T, Allocator>::value_type*
    basic_string<T, Allocator>::CharTypeStringRFind(const value_type* pRBegin, const value_type* pREnd, const value_type c)
    {
        while(pRBegin > pREnd)
        {
            if(*(pRBegin - 1) == c)
                return pRBegin;
            --pRBegin;
        }
        return pREnd;
    }


    // CharTypeStringSearch
    // Specialized value_type version of STL search() function.
    // Purpose: find p2 within p1. Return p1End if not found or if either string is zero length.
    template <typename T, typename Allocator>
    const typename basic_string<T, Allocator>::value_type*
    basic_string<T, Allocator>::CharTypeStringSearch(const value_type* p1Begin, const value_type* p1End, 
                                                     const value_type* p2Begin, const value_type* p2End)
    {
        // Test for zero length strings, in which case we have a match or a failure, 
        // but the return value is the same either way.
        if((p1Begin == p1End) || (p2Begin == p2End))
            return p1Begin;

        // Test for a pattern of length 1.
        if((p2Begin + 1) == p2End)
            return eastl::find(p1Begin, p1End, *p2Begin);

        // General case.
        const value_type* pTemp;
        const value_type* pTemp1 = (p2Begin + 1);
        const value_type* pCurrent = p1Begin;

        while(p1Begin != p1End)
        {
            p1Begin = eastl::find(p1Begin, p1End, *p2Begin);
            if(p1Begin == p1End)
                return p1End;

            pTemp = pTemp1;
            pCurrent = p1Begin; 
            if(++pCurrent == p1End)
                return p1End;

            while(*pCurrent == *pTemp)
            {
                if(++pTemp == p2End)
                    return p1Begin;
                if(++pCurrent == p1End)
                    return p1End;
            }

            ++p1Begin;
        }

        return p1Begin;
    }


    // CharTypeStringRSearch
    // Specialized value_type version of STL find_end() function (which really is a reverse search function).
    // Purpose: find last instance of p2 within p1. Return p1End if not found or if either string is zero length.
    template <typename T, typename Allocator>
    const typename basic_string<T, Allocator>::value_type* 
    basic_string<T, Allocator>::CharTypeStringRSearch(const value_type* p1Begin, const value_type* p1End, 
                                                      const value_type* p2Begin, const value_type* p2End)
    {
        // Test for zero length strings, in which case we have a match or a failure, 
        // but the return value is the same either way.
        if((p1Begin == p1End) || (p2Begin == p2End))
            return p1Begin;

        // Test for a pattern of length 1.
        if((p2Begin + 1) == p2End)
            return CharTypeStringFindEnd(p1Begin, p1End, *p2Begin);

        // Test for search string length being longer than string length.
        if((p2End - p2Begin) > (p1End - p1Begin))
            return p1End;

        // General case.
        const value_type* pSearchEnd = (p1End - (p2End - p2Begin) + 1);
        const value_type* pCurrent1;
        const value_type* pCurrent2;

        while(pSearchEnd != p1Begin)
        {
            // Search for the last occurrence of *p2Begin.
            pCurrent1 = CharTypeStringFindEnd(p1Begin, pSearchEnd, *p2Begin);
            if(pCurrent1 == pSearchEnd) // If the first char of p2 wasn't found, 
                return p1End;           // then we immediately have failure.

            // In this case, *pTemp == *p2Begin. So compare the rest.
            pCurrent2 = p2Begin;
            while(*pCurrent1++ == *pCurrent2++)
            {
                if(pCurrent2 == p2End)
                    return (pCurrent1 - (p2End - p2Begin));
            }

            // A smarter algorithm might know to subtract more than just one,
            // but in most cases it won't make much difference anyway.
            --pSearchEnd;
        }

        return p1End;
    }


    // CharTypeStringFindFirstOf
    // Specialized value_type version of STL find_first_of() function.
    // This function is much like the C runtime strtok function, except the strings aren't null-terminated.
    template <typename T, typename Allocator>
    const typename basic_string<T, Allocator>::value_type*
    basic_string<T, Allocator>::CharTypeStringFindFirstOf(const value_type* p1Begin, const value_type* p1End, 
                                                          const value_type* p2Begin, const value_type* p2End)
    {
        for( ; p1Begin != p1End; ++p1Begin)
        {
            for(const value_type* pTemp = p2Begin; pTemp != p2End; ++pTemp)
            {
                if(*p1Begin == *pTemp)
                    return p1Begin;
            }
        }
        return p1End;
    }


    // CharTypeStringRFindFirstOf
    // Specialized value_type version of STL find_first_of() function in reverse.
    // This function is much like the C runtime strtok function, except the strings aren't null-terminated.
    template <typename T, typename Allocator>
    const typename basic_string<T, Allocator>::value_type*
    basic_string<T, Allocator>::CharTypeStringRFindFirstOf(const value_type* p1RBegin, const value_type* p1REnd, 
                                                           const value_type* p2Begin,  const value_type* p2End)
    {
        for( ; p1RBegin != p1REnd; --p1RBegin)
        {
            for(const value_type* pTemp = p2Begin; pTemp != p2End; ++pTemp)
            {
                if(*(p1RBegin - 1) == *pTemp)
                    return p1RBegin;
            }
        }
        return p1REnd;
    }



    // CharTypeStringFindFirstNotOf
    // Specialized value_type version of STL find_first_not_of() function.
    template <typename T, typename Allocator>
    const typename basic_string<T, Allocator>::value_type*
    basic_string<T, Allocator>::CharTypeStringFindFirstNotOf(const value_type* p1Begin, const value_type* p1End, 
                                                             const value_type* p2Begin, const value_type* p2End)
    {
        for( ; p1Begin != p1End; ++p1Begin)
        {
            const value_type* pTemp;
            for(pTemp = p2Begin; pTemp != p2End; ++pTemp)
            {
                if(*p1Begin == *pTemp)
                    break;
            }
            if(pTemp == p2End)
                return p1Begin;
        }
        return p1End;
    }


    // CharTypeStringRFindFirstNotOf
    // Specialized value_type version of STL find_first_not_of() function in reverse.
    template <typename T, typename Allocator>
    const typename basic_string<T, Allocator>::value_type*
    basic_string<T, Allocator>::CharTypeStringRFindFirstNotOf(const value_type* p1RBegin, const value_type* p1REnd, 
                                                              const value_type* p2Begin,  const value_type* p2End)
    {
        for( ; p1RBegin != p1REnd; --p1RBegin)
        {
            const value_type* pTemp;
            for(pTemp = p2Begin; pTemp != p2End; ++pTemp)
            {
                if(*(p1RBegin-1) == *pTemp)
                    break;
            }
            if(pTemp == p2End)
                return p1RBegin;
        }
        return p1REnd;
    }




    // iterator operators
    template <typename T, typename Allocator>
    inline bool operator==(const typename basic_string<T, Allocator>::reverse_iterator& r1, 
                           const typename basic_string<T, Allocator>::reverse_iterator& r2)
    {
        return r1.mpCurrent == r2.mpCurrent;
    }


    template <typename T, typename Allocator>
    inline bool operator!=(const typename basic_string<T, Allocator>::reverse_iterator& r1, 
                           const typename basic_string<T, Allocator>::reverse_iterator& r2)
    {
        return r1.mpCurrent != r2.mpCurrent;
    }


    // Operator +
    template <typename T, typename Allocator>
    basic_string<T, Allocator> operator+(const basic_string<T, Allocator>& a, const basic_string<T, Allocator>& b)
    {
        typedef typename basic_string<T, Allocator>::CtorDoNotInitialize CtorDoNotInitialize;
        CtorDoNotInitialize cDNI; // GCC 2.x forces us to declare a named temporary like this.
        basic_string<T, Allocator> result(cDNI, a.size() + b.size(), const_cast<basic_string<T, Allocator>&>(a).get_allocator()); // Note that we choose to assign a's allocator.
        result.append(a);
        result.append(b);
        return result;
    }


    template <typename T, typename Allocator>
    basic_string<T, Allocator> operator+(const typename basic_string<T, Allocator>::value_type* p, const basic_string<T, Allocator>& b)
    {
        typedef typename basic_string<T, Allocator>::CtorDoNotInitialize CtorDoNotInitialize;
        CtorDoNotInitialize cDNI; // GCC 2.x forces us to declare a named temporary like this.
        const typename basic_string<T, Allocator>::size_type n = (typename basic_string<T, Allocator>::size_type)CharStrlen(p);
        basic_string<T, Allocator> result(cDNI, n + b.size(), const_cast<basic_string<T, Allocator>&>(b).get_allocator());
        result.append(p, p + n);
        result.append(b);
        return result;
    }


    template <typename T, typename Allocator>
    basic_string<T, Allocator> operator+(typename basic_string<T, Allocator>::value_type c, const basic_string<T, Allocator>& b)
    {
        typedef typename basic_string<T, Allocator>::CtorDoNotInitialize CtorDoNotInitialize;
        CtorDoNotInitialize cDNI; // GCC 2.x forces us to declare a named temporary like this.
        basic_string<T, Allocator> result(cDNI, 1 + b.size(), const_cast<basic_string<T, Allocator>&>(b).get_allocator());
        result.push_back(c);
        result.append(b);
        return result;
    }


    template <typename T, typename Allocator>
    basic_string<T, Allocator> operator+(const basic_string<T, Allocator>& a, const typename basic_string<T, Allocator>::value_type* p)
    {
        typedef typename basic_string<T, Allocator>::CtorDoNotInitialize CtorDoNotInitialize;
        CtorDoNotInitialize cDNI; // GCC 2.x forces us to declare a named temporary like this.
        const typename basic_string<T, Allocator>::size_type n = (typename basic_string<T, Allocator>::size_type)CharStrlen(p);
        basic_string<T, Allocator> result(cDNI, a.size() + n, const_cast<basic_string<T, Allocator>&>(a).get_allocator());
        result.append(a);
        result.append(p, p + n);
        return result;
    }


    template <typename T, typename Allocator>
    basic_string<T, Allocator> operator+(const basic_string<T, Allocator>& a, const typename basic_string<T, Allocator>::value_type c)
    {
        typedef typename basic_string<T, Allocator>::CtorDoNotInitialize CtorDoNotInitialize;
        CtorDoNotInitialize cDNI; // GCC 2.x forces us to declare a named temporary like this.
        basic_string<T, Allocator> result(cDNI, a.size() + 1, const_cast<basic_string<T, Allocator>&>(a).get_allocator());
        result.append(a);
        result.push_back(c);
        return result;
    }


    template <typename T, typename Allocator>
    inline bool basic_string<T, Allocator>::validate() const
    {
        if((mpBegin == NULL) || (mpEnd == NULL))
            return false;
        if(mpEnd < mpBegin)
            return false;
        if(mpCapacity < mpEnd)
            return false;
        return true;
    }


    template <typename T, typename Allocator>
    inline int basic_string<T, Allocator>::validate_iterator(const_iterator i) const
    {
        if(i >= mpBegin)
        {
            if(i < mpEnd)
                return (isf_valid | isf_current | isf_can_dereference);

            if(i <= mpEnd)
                return (isf_valid | isf_current);
        }

        return isf_none;
    }


    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    // Operator== and operator!=
    template <typename T, typename Allocator>
    inline bool operator==(const basic_string<T, Allocator>& a, const basic_string<T, Allocator>& b)
    {
        return ((a.size() == b.size()) && (memcmp(a.data(), b.data(), (size_t)a.size() * sizeof(typename basic_string<T, Allocator>::value_type)) == 0));
    }


    template <typename T, typename Allocator>
    inline bool operator==(const typename basic_string<T, Allocator>::value_type* p, const basic_string<T, Allocator>& b)
    {
        typedef typename basic_string<T, Allocator>::size_type size_type;
        const size_type n = (size_type)CharStrlen(p);
        return ((n == b.size()) && (memcmp(p, b.data(), (size_t)n * sizeof(*p)) == 0));
    }


    template <typename T, typename Allocator>
    inline bool operator==(const basic_string<T, Allocator>& a, const typename basic_string<T, Allocator>::value_type* p)
    {
        typedef typename basic_string<T, Allocator>::size_type size_type;
        const size_type n = (size_type)CharStrlen(p);
        return ((a.size() == n) && (memcmp(a.data(), p, (size_t)n * sizeof(*p)) == 0));
    }


    template <typename T, typename Allocator>
    inline bool operator!=(const basic_string<T, Allocator>& a, const basic_string<T, Allocator>& b)
    {
        return !(a == b);
    }


    template <typename T, typename Allocator>
    inline bool operator!=(const typename basic_string<T, Allocator>::value_type* p, const basic_string<T, Allocator>& b)
    {
        return !(p == b);
    }


    template <typename T, typename Allocator>
    inline bool operator!=(const basic_string<T, Allocator>& a, const typename basic_string<T, Allocator>::value_type* p)
    {
        return !(a == p);
    }


    // Operator< (and also >, <=, and >=).
    template <typename T, typename Allocator>
    inline bool operator<(const basic_string<T, Allocator>& a, const basic_string<T, Allocator>& b)
    {
        return basic_string<T, Allocator>::compare(a.begin(), a.end(), b.begin(), b.end()) < 0; }


    template <typename T, typename Allocator>
    inline bool operator<(const typename basic_string<T, Allocator>::value_type* p, const basic_string<T, Allocator>& b)
    {
        typedef typename basic_string<T, Allocator>::size_type size_type;
        const size_type n = (size_type)CharStrlen(p);
        return basic_string<T, Allocator>::compare(p, p + n, b.begin(), b.end()) < 0;
    }


    template <typename T, typename Allocator>
    inline bool operator<(const basic_string<T, Allocator>& a, const typename basic_string<T, Allocator>::value_type* p)
    {
        typedef typename basic_string<T, Allocator>::size_type size_type;
        const size_type n = (size_type)CharStrlen(p);
        return basic_string<T, Allocator>::compare(a.begin(), a.end(), p, p + n) < 0;
    }


    template <typename T, typename Allocator>
    inline bool operator>(const basic_string<T, Allocator>& a, const basic_string<T, Allocator>& b)
    {
        return b < a;
    }


    template <typename T, typename Allocator>
    inline bool operator>(const typename basic_string<T, Allocator>::value_type* p, const basic_string<T, Allocator>& b)
    {
        return b < p;
    }


    template <typename T, typename Allocator>
    inline bool operator>(const basic_string<T, Allocator>& a, const typename basic_string<T, Allocator>::value_type* p)
    {
        return p < a;
    }


    template <typename T, typename Allocator>
    inline bool operator<=(const basic_string<T, Allocator>& a, const basic_string<T, Allocator>& b)
    {
        return !(b < a);
    }


    template <typename T, typename Allocator>
    inline bool operator<=(const typename basic_string<T, Allocator>::value_type* p, const basic_string<T, Allocator>& b)
    {
        return !(b < p);
    }


    template <typename T, typename Allocator>
    inline bool operator<=(const basic_string<T, Allocator>& a, const typename basic_string<T, Allocator>::value_type* p)
    {
        return !(p < a);
    }


    template <typename T, typename Allocator>
    inline bool operator>=(const basic_string<T, Allocator>& a, const basic_string<T, Allocator>& b)
    {
        return !(a < b);
    }


    template <typename T, typename Allocator>
    inline bool operator>=(const typename basic_string<T, Allocator>::value_type* p, const basic_string<T, Allocator>& b)
    {
        return !(p < b);
    }


    template <typename T, typename Allocator>
    inline bool operator>=(const basic_string<T, Allocator>& a, const typename basic_string<T, Allocator>::value_type* p)
    {
        return !(a < p);
    }


    template <typename T, typename Allocator>
    inline void swap(basic_string<T, Allocator>& a, basic_string<T, Allocator>& b)
    {
        a.swap(b);
    }


    /// string / wstring
    typedef basic_string<char>    string;
    typedef basic_string<wchar_t> wstring;

    /// string8 / string16 / string32
    typedef basic_string<char8_t>  string8;
    typedef basic_string<char16_t> string16;
    typedef basic_string<char32_t> string32;



    /// hash<string>
    ///
    /// We provide EASTL hash function objects for use in hash table containers.
    ///
    /// Example usage:
    ///    #include <EASTL/hash_set.h>
    ///    hash_set<string> stringHashSet;
    ///
    template <typename T> struct hash;

    template <>
    struct hash<string>
    {
        size_t operator()(const string& x) const
        {
            const unsigned char* p = (const unsigned char*)x.c_str(); // To consider: limit p to at most 256 chars.
            unsigned int c, result = 2166136261U; // We implement an FNV-like string hash. 
            while((c = *p++) != 0) // Using '!=' disables compiler warnings.
                result = (result * 16777619) ^ c;
            return (size_t)result;
        }
    };

    /// hash<wstring>
    ///
    template <>
    struct hash<wstring>
    {
        size_t operator()(const wstring& x) const
        {
            const wchar_t* p = (const wchar_t*)x.c_str(); // To consider: limit p to at most 256 chars.
            unsigned int c, result = 2166136261U; // We implement an FNV-like string hash. 
            while((c = *p++) != 0) // Using '!=' disables compiler warnings.
                result = (result * 16777619) ^ c;
            return (size_t)result;
        }
    };


} // namespace eastl


#ifdef _MSC_VER
    #pragma warning(pop)
#endif

#endif // EASTL_ABSTRACT_STRING_ENABLED

#endif // Header include guard























