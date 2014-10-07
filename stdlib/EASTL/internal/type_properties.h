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
// EASTL/internal/type_properties.h
// Written and maintained by Paul Pedriana - 2005.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_INTERNAL_TYPE_PROPERTIES_H
#define EASTL_INTERNAL_TYPE_PROPERTIES_H


#include <limits.h>


namespace eastl
{

    // The following properties or relations are defined here. If the given 
    // item is missing then it simply hasn't been implemented, at least not yet.

    ///////////////////////////////////////////////////////////////////////
    // is_const
    //
    // is_const<T>::value == true if and only if T has const-qualification.
    //
    ///////////////////////////////////////////////////////////////////////
    template <typename T> struct is_const_value                    : public false_type{};
    template <typename T> struct is_const_value<const T*>          : public true_type{};
    template <typename T> struct is_const_value<const volatile T*> : public true_type{};

    template <typename T> struct is_const : public is_const_value<T*>{};
    template <typename T> struct is_const<T&> : public false_type{}; // Note here that T is const, not the reference to T. So is_const is false. See section 8.3.2p1 of the C++ standard.



    ///////////////////////////////////////////////////////////////////////
    // is_volatile
    //
    // is_volatile<T>::value == true  if and only if T has volatile-qualification.
    //
    ///////////////////////////////////////////////////////////////////////

    template <typename T> struct is_volatile_value                    : public false_type{};
    template <typename T> struct is_volatile_value<volatile T*>       : public true_type{};
    template <typename T> struct is_volatile_value<const volatile T*> : public true_type{};

    template <typename T> struct is_volatile : public is_volatile_value<T*>{};
    template <typename T> struct is_volatile<T&> : public false_type{}; // Note here that T is volatile, not the reference to T. So is_const is false. See section 8.3.2p1 of the C++ standard.



    ///////////////////////////////////////////////////////////////////////
    // is_abstract
    //
    // is_abstract<T>::value == true if and only if T is a class or struct 
    // that has at least one pure virtual function. is_abstract may only 
    // be applied to complete types.
    //
    ///////////////////////////////////////////////////////////////////////

    // Not implemented yet.



    ///////////////////////////////////////////////////////////////////////
    // is_signed
    //
    // is_signed<T>::value == true if and only if T is one of the following types:
    //    [const] [volatile] char (maybe)
    //    [const] [volatile] signed char
    //    [const] [volatile] short
    //    [const] [volatile] int
    //    [const] [volatile] long
    //    [const] [volatile] long long
    // 
    // Used to determine if a integral type is signed or unsigned.
    // Given that there are some user-made classes which emulate integral
    // types, we provide the EASTL_DECLARE_SIGNED macro to allow you to 
    // set a given class to be identified as a signed type.
    ///////////////////////////////////////////////////////////////////////
    template <typename T> struct is_signed : public false_type{};
    
    #define EASTL_TMP_DECLARE_SIGNED_WITH_CV(T)\
        template <> struct is_signed<T>                : public true_type{};\
        template <> struct is_signed<T const>          : public true_type{};\
        template <> struct is_signed<T volatile>       : public true_type{};\
        template <> struct is_signed<T const volatile> : public true_type{};
    
    EASTL_TMP_DECLARE_SIGNED_WITH_CV(signed char)
    EASTL_TMP_DECLARE_SIGNED_WITH_CV(signed short)
    EASTL_TMP_DECLARE_SIGNED_WITH_CV(signed int)
    EASTL_TMP_DECLARE_SIGNED_WITH_CV(signed long)
    EASTL_TMP_DECLARE_SIGNED_WITH_CV(signed long long)
    
    #if (CHAR_MAX == SCHAR_MAX)
        EASTL_TMP_DECLARE_SIGNED_WITH_CV(char)
    #endif
    #ifndef EA_WCHAR_T_NON_NATIVE // If wchar_t is a native type instead of simply a define to an existing type...
        #if defined(__WCHAR_MAX__) && ((__WCHAR_MAX__ == 2147483647) || (__WCHAR_MAX__ == 32767)) // GCC defines __WCHAR_MAX__ for most platforms.
            EASTL_TMP_DECLARE_SIGNED_WITH_CV(wchar_t)
        #endif
    #endif
    
    #undef EASTL_TMP_DECLARE_SIGNED_WITH_CV
    
    #define EASTL_DECLARE_SIGNED(T) namespace eastl{ template <> struct is_signed<T> : public true_type{}; template <> struct is_signed<const T> : public true_type{}; }



    ///////////////////////////////////////////////////////////////////////
    // is_unsigned
    //
    // is_unsigned<T>::value == true if and only if T is one of the following types:
    //    [const] [volatile] char (maybe)
    //    [const] [volatile] unsigned char
    //    [const] [volatile] unsigned short
    //    [const] [volatile] unsigned int
    //    [const] [volatile] unsigned long
    //    [const] [volatile] unsigned long long
    //
    // Used to determine if a integral type is signed or unsigned. 
    // Given that there are some user-made classes which emulate integral
    // types, we provide the EASTL_DECLARE_UNSIGNED macro to allow you to 
    // set a given class to be identified as an unsigned type.
    ///////////////////////////////////////////////////////////////////////
    template <typename T> struct is_unsigned : public false_type{};
    
    #define EASTL_TMP_DECLARE_UNSIGNED_WITH_CV(T)\
        template <> struct is_unsigned<T>                : public true_type{};\
        template <> struct is_unsigned<T const>          : public true_type{};\
        template <> struct is_unsigned<T volatile>       : public true_type{};\
        template <> struct is_unsigned<T const volatile> : public true_type{};
    
    EASTL_TMP_DECLARE_UNSIGNED_WITH_CV(unsigned char)
    EASTL_TMP_DECLARE_UNSIGNED_WITH_CV(unsigned short)
    EASTL_TMP_DECLARE_UNSIGNED_WITH_CV(unsigned int)
    EASTL_TMP_DECLARE_UNSIGNED_WITH_CV(unsigned long)
    EASTL_TMP_DECLARE_UNSIGNED_WITH_CV(unsigned long long)
    
    #if (CHAR_MAX == UCHAR_MAX)
        EASTL_TMP_DECLARE_UNSIGNED_WITH_CV(char)
    #endif
    #ifndef EA_WCHAR_T_NON_NATIVE // If wchar_t is a native type instead of simply a define to an existing type...
        #if defined(_MSC_VER) || (defined(__WCHAR_MAX__) && ((__WCHAR_MAX__ == 4294967295U) || (__WCHAR_MAX__ == 65535))) // GCC defines __WCHAR_MAX__ for most platforms.
            EASTL_TMP_DECLARE_UNSIGNED_WITH_CV(wchar_t)
        #endif
    #endif
    
    #undef EASTL_TMP_DECLARE_UNSIGNED_WITH_CV
    
    #define EASTL_DECLARE_UNSIGNED(T) namespace eastl{ template <> struct is_unsigned<T> : public true_type{}; template <> struct is_unsigned<const T> : public true_type{}; }



    ///////////////////////////////////////////////////////////////////////
    // alignment_of
    //
    // alignment_of<T>::value is an integral value representing, in bytes, 
    // the memory alignment of objects of type T.
    //
    // alignment_of may only be applied to complete types.
    //
    ///////////////////////////////////////////////////////////////////////
    template <typename T>
    struct alignment_of_value{ static const size_t value = EASTL_ALIGN_OF(T); };

    template <typename T>
    struct alignment_of : public integral_constant<size_t, alignment_of_value<T>::value>{};



    ///////////////////////////////////////////////////////////////////////
    // is_aligned
    //
    // Defined as true if the type has alignment requirements greater 
    // than default alignment, which is taken to be 8. This allows for 
    // doing specialized object allocation and placement for such types.
    ///////////////////////////////////////////////////////////////////////
    template <typename T>
    struct is_aligned_value{ static const bool value = (EASTL_ALIGN_OF(T) > 8); };

    template <typename T> 
    struct is_aligned : public integral_constant<bool, is_aligned_value<T>::value>{};



    ///////////////////////////////////////////////////////////////////////
    // rank
    //
    // rank<T>::value is an integral value representing the number of 
    // dimensions possessed by an array type. For example, given a 
    // multi-dimensional array type T[M][N], std::tr1::rank<T[M][N]>::value == 2. 
    // For a given non-array type T, std::tr1::rank<T>::value == 0.
    //
    ///////////////////////////////////////////////////////////////////////

    // Not implemented yet.



    ///////////////////////////////////////////////////////////////////////
    // extent
    //
    // extent<T, I>::value is an integral type representing the number of 
    // elements in the Ith dimension of array type T.
    // 
    // For a given array type T[N], std::tr1::extent<T[N]>::value == N.
    // For a given multi-dimensional array type T[M][N], std::tr1::extent<T[M][N], 0>::value == N.
    // For a given multi-dimensional array type T[M][N], std::tr1::extent<T[M][N], 1>::value == M.
    // For a given array type T and a given dimension I where I >= rank<T>::value, std::tr1::extent<T, I>::value == 0.
    // For a given array type of unknown extent T[], std::tr1::extent<T[], 0>::value == 0.
    // For a given non-array type T and an arbitrary dimension I, std::tr1::extent<T, I>::value == 0.
    // 
    ///////////////////////////////////////////////////////////////////////

    // Not implemented yet.



    ///////////////////////////////////////////////////////////////////////
    // is_base_of
    //
    // Given two (possibly identical) types Base and Derived, is_base_of<Base, Derived>::value == true 
    // if and only if Base is a direct or indirect base class of Derived, 
    // or Base and Derived are the same type.
    //
    // is_base_of may only be applied to complete types.
    //
    ///////////////////////////////////////////////////////////////////////

    // Not implemented yet.



} // namespace eastl


#endif // Header include guard




















