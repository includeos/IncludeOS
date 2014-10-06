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
// EASTL/internal/type_transformations.h
// Written and maintained by Paul Pedriana - 2005
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_INTERNAL_TYPE_TRANFORMATIONS_H
#define EASTL_INTERNAL_TYPE_TRANFORMATIONS_H


namespace eastl
{


    // The following transformations are defined here. If the given item 
    // is missing then it simply hasn't been implemented, at least not yet.
    //    add_unsigned
    //    add_signed
    //    remove_const
    //    remove_volatile
    //    remove_cv
    //    add_const
    //    add_volatile
    //    add_cv
    //    remove_reference
    //    add_reference
    //    remove_extent
    //    remove_all_extents
    //    remove_pointer
    //    add_pointer
    //    aligned_storage


    ///////////////////////////////////////////////////////////////////////
    // add_signed
    //
    // Adds signed-ness to the given type. 
    // Modifies only integral values; has no effect on others.
    // add_signed<int>::type is int
    // add_signed<unsigned int>::type is int
    //
    ///////////////////////////////////////////////////////////////////////

    template<class T>
    struct add_signed
    { typedef T type; };

    template<>
    struct add_signed<unsigned char>
    { typedef signed char type; };

    #if (defined(CHAR_MAX) && defined(UCHAR_MAX) && (CHAR_MAX == UCHAR_MAX)) // If char is unsigned (which is usually not the case)...
        template<>
        struct add_signed<char>
        { typedef signed char type; };
    #endif

    template<>
    struct add_signed<unsigned short>
    { typedef short type; };

    template<>
    struct add_signed<unsigned int>
    { typedef int type; };

    template<>
    struct add_signed<unsigned long>
    { typedef long type; };

    template<>
    struct add_signed<unsigned long long>
    { typedef long long type; };

    #ifndef EA_WCHAR_T_NON_NATIVE // If wchar_t is a native type instead of simply a define to an existing type...
        #if (defined(__WCHAR_MAX__) && (__WCHAR_MAX__ == 4294967295U)) // If wchar_t is a 32 bit unsigned value...
            template<>
            struct add_signed<wchar_t>
            { typedef int32_t type; };
        #elif (defined(__WCHAR_MAX__) && (__WCHAR_MAX__ == 65535)) // If wchar_t is a 16 bit unsigned value...
            template<>
            struct add_signed<wchar_t>
            { typedef int16_t type; };
        #endif
    #endif



    ///////////////////////////////////////////////////////////////////////
    // add_unsigned
    //
    // Adds unsigned-ness to the given type. 
    // Modifies only integral values; has no effect on others.
    // add_unsigned<int>::type is unsigned int
    // add_unsigned<unsigned int>::type is unsigned int
    //
    ///////////////////////////////////////////////////////////////////////

    template<class T>
    struct add_unsigned
    { typedef T type; };

    template<>
    struct add_unsigned<signed char>
    { typedef unsigned char type; };

    #if (defined(CHAR_MAX) && defined(SCHAR_MAX) && (CHAR_MAX == SCHAR_MAX)) // If char is signed (which is usually so)...
        template<>
        struct add_unsigned<char>
        { typedef unsigned char type; };
    #endif

    template<>
    struct add_unsigned<short>
    { typedef unsigned short type; };

    template<>
    struct add_unsigned<int>
    { typedef unsigned int type; };

    template<>
    struct add_unsigned<long>
    { typedef unsigned long type; };

    template<>
    struct add_unsigned<long long>
    { typedef unsigned long long type; };

    #ifndef EA_WCHAR_T_NON_NATIVE // If wchar_t is a native type instead of simply a define to an existing type...
        #if (defined(__WCHAR_MAX__) && (__WCHAR_MAX__ == 2147483647)) // If wchar_t is a 32 bit signed value...
            template<>
            struct add_unsigned<wchar_t>
            { typedef uint32_t type; };
        #elif (defined(__WCHAR_MAX__) && (__WCHAR_MAX__ == 32767)) // If wchar_t is a 16 bit signed value...
            template<>
            struct add_unsigned<wchar_t>
            { typedef uint16_t type; };
        #endif
    #endif

    ///////////////////////////////////////////////////////////////////////
    // remove_cv
    //
    // Remove const and volatile from a type.
    //
    // The remove_cv transformation trait removes top-level const and/or volatile 
    // qualification (if any) from the type to which it is applied. For a given type T, 
    // remove_cv<T const volatile>::type is equivalent to T. For example, 
    // remove_cv<char* volatile>::type is equivalent to char*, while remove_cv<const char*>::type 
    // is equivalent to const char*. In the latter case, the const qualifier modifies 
    // char, not *, and is therefore not at the top level.
    //
    ///////////////////////////////////////////////////////////////////////
    template <typename T> struct remove_cv_imp{};
    template <typename T> struct remove_cv_imp<T*>                { typedef T unqualified_type; };
    template <typename T> struct remove_cv_imp<const T*>          { typedef T unqualified_type; };
    template <typename T> struct remove_cv_imp<volatile T*>       { typedef T unqualified_type; };
    template <typename T> struct remove_cv_imp<const volatile T*> { typedef T unqualified_type; };

    template <typename T> struct remove_cv{ typedef typename remove_cv_imp<T*>::unqualified_type type; };
    template <typename T> struct remove_cv<T&>{ typedef T& type; }; // References are automatically not const nor volatile. See section 8.3.2p1 of the C++ standard.

    template <typename T, size_t N> struct remove_cv<T const[N]>         { typedef T type[N]; };
    template <typename T, size_t N> struct remove_cv<T volatile[N]>      { typedef T type[N]; };
    template <typename T, size_t N> struct remove_cv<T const volatile[N]>{ typedef T type[N]; };


  
    ///////////////////////////////////////////////////////////////////////
    // add_reference
    //
    // Add reference to a type.
    //
    // The add_reference transformation trait adds a level of indirection 
    // by reference to the type to which it is applied. For a given type T, 
    // add_reference<T>::type is equivalent to T& if is_reference<T>::value == false, 
    // and T otherwise.
    //
    ///////////////////////////////////////////////////////////////////////
    template <typename T>
    struct add_reference_impl{ typedef T& type; };

    template <typename T>
    struct add_reference_impl<T&>{ typedef T& type; };

    template <>
    struct add_reference_impl<void>{ typedef void type; };

    template <typename T>
    struct add_reference { typedef typename add_reference_impl<T>::type type; };


} // namespace eastl


#endif // Header include guard





















