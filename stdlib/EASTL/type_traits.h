/*
Copyright (C) 2009-2010 Electronic Arts, Inc.  All rights reserved.

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
// EASTL/type_traits.h
//
// Copyright (c) 2005, Electronic Arts. All rights reserved.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Specification
//
// This file implements C++ type traits as proposed by the emerging C++ update
// as of May, 2005. This update is known as "Proposed Draft Technical Report 
// on C++ Library Extensions" and is document number n1745. It can be found 
// on the Internet as n1745.pdf and as of this writing it is updated every 
// couple months to reflect current thinking.
//////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Description
//
// EASTL includes a fairly serious type traits library that is on par with the 
// one found in Boost but offers some additional performance-enhancing help as well. 
// The type_traits library provides information about class types, as opposed to 
// class instances. For example, the is_integral type trait tells if a type is 
// one of int, short, long, char, uint64_t, etc.
//
// There are three primary uses of type traits:
//     * Allowing for optimized operations on some data types.
//     * Allowing for different logic pathways based on data types.
//     * Allowing for compile-type assertions about data type expectations. 
// 
// Most of the type traits are automatically detected and implemented by the compiler. 
// However, EASTL allows for the user to explicitly give the compiler hints about 
// type traits that the compiler cannot know, via the EASTL_DECLARE declarations. 
// If the user has a class that is relocatable (i.e. can safely use memcpy to copy values), 
// the user can use the EASTL_DECLARE_TRIVIAL_RELOCATE declaration to tell the compiler 
// that the class can be copied via memcpy. This will automatically significantly speed 
// up some containers and algorithms that use that class.
// 
// Here is an example of using type traits to tell if a value is a floating point 
// value or not:
// 
//    template <typename T>
//    DoSomething(T t) {
//        assert(is_floating_point<T>::value);
//    }
// 
// Here is an example of declaring a class as relocatable and using it in a vector.
// 
//    EASTL_DECLARE_TRIVIAL_RELOCATE(Widget); // Usually you put this at the Widget class declaration.
//    vector<Widget> wVector;
//    wVector.erase(wVector.begin());         // This operation will be optimized via using memcpy.
// 
// The following is a full list of the currently recognized type traits. Most of these 
// are implemented as of this writing, but if there is one that is missing, feel free 
// to contact the maintainer of this library and request that it be completed.
// 
//    Trait                             Description
// ------------------------------------------------------------------------------
//    is_void                           T is void or a cv-qualified (const/void-qualified) void.
//    is_integral                       T is an integral type.
//    is_floating_point                 T is a floating point type.
//    is_arithmetic                     T is an arithmetic type (integral or floating point).
//    is_fundamental                    T is a fundamental type (void, integral, or floating point).
//    is_const                          T is const-qualified.
//    is_volatile                       T is volatile-qualified.
//    is_abstract                       T is an abstract class.
//    is_signed                         T is a signed integral type.
//    is_unsigned                       T is an unsigned integral type.
//    is_array                          T is an array type. The templated array container is not an array type.
//    is_pointer                        T is a pointer type. Includes function pointers, but not pointers to (data or function) members.
//    is_reference                      T is a reference type. Includes references to functions.
//    is_member_object_pointer          T is a pointer to data member.
//    is_member_function_pointer        T is a pointer to member function.
//    is_member_pointer                 T is a pointer to a member or member function.
//    is_enum                           T is an enumeration type.
//    is_union                          T is a union type.
//    is_class                          T is a class type but not a union type.
//    is_polymorphic                    T is a polymorphic class.
//    is_function                       T is a function type.
//    is_object                         T is an object type.
//    is_scalar                         T is a scalar type (arithmetic, enum, pointer, member_pointer)
//    is_compound                       T is a compound type (anything but fundamental).
//    is_same                           T and U name the same type.
//    is_convertible                    An imaginary lvalue of type From is implicitly convertible to type To. Special conversions involving string-literals and null-pointer constants are not considered. No function-parameter adjustments are made to type To when determining whether From is convertible to To; this implies that if type To is a function type or an array type, then the condition is false.
//    is_base_of                        Base is a base class of Derived or Base and Derived name the same type.
//    is_empty                          T is an empty class.
//    is_pod                            T is a POD type.
//   *is_aligned                        Defined as true if the type has alignment requirements greater than default alignment, which is taken to be 8.
//    has_trivial_constructor           The default constructor for T is trivial.
//    has_trivial_copy                  The copy constructor for T is trivial.
//    has_trivial_assign                The assignment operator for T is trivial.
//    has_trivial_destructor            The destructor for T is trivial.
//   *has_trivial_relocate              T can be moved to a new location via bitwise copy.
//    has_nothrow_constructor           The default constructor for T has an empty exception specification or can otherwise be deduced never to throw an exception.
//    has_nothrow_copy                  The copy constructor for T has an empty exception specification or can otherwise be deduced never to throw an exception.
//    has_nothrow_assign                The assignment operator for T has an empty exception specification or can otherwise be deduced never to throw an exception.
//    has_virtual_destructor            T has a virtual destructor.
//    alignment_of                      An integer value representing the number of bytes of the alignment of objects of type T; an object of type T may be allocated at an address that is a multiple of its alignment.
//    rank                              An integer value representing the rank of objects of type T. The term 'rank' here is used to describe the number of dimensions of an array type.
//    extent                            An integer value representing the extent (dimension) of the I'th bound of objects of type T. If the type T is not an array type, has rank of less than I, or if I == 0 and T is of type 'array of unknown bound of U,' then value shall evaluate to zero; otherwise value shall evaluate to the number of elements in the I'th array bound of T. The term 'extent' here is used to describe the number of elements in an array type.
//    remove_const                      The member typedef type shall be the same as T except that any top level const-qualifier has been removed. remove_const<const volatile int>::type evaluates to volatile int, whereas remove_const<const int*> is const int*.
//
// * is_aligned is not found in Boost nor the C++ standard update proposal.
//
// * has_trivial_relocate is not found in Boost nor the C++ standard update proposal. 
//   However, it is very useful in allowing for the generation of optimized object 
//   moving operations. It is similar to the is_pod type trait, but goes further and 
//   allows non-pod classes to be categorized as relocatable. Such categorization is 
//   something that no compiler can do, as only the user can know if it is such. 
//   Thus EASTL_DECLARE_TRIVIAL_RELOCATE is provided to allow the user to give 
//   the compiler a hint.
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Requirements
//
// As of this writing (5/2005), type_traits here requires a well-conforming 
// C++ compiler with respect to template metaprogramming. To use this library
// you need to have at least one of the following:
//     MSVC++ 7.1       (includes Win32, XBox 360, Win64, and WinCE platforms)
//     GCC 3.2          (includes Playstation 3, and Linux platforms)
//     Metrowerks 8.0   (incluees Playstation 3, Windows, and other platforms)
//     SN Systems       (not the GCC 2.95-based compilers)
//     EDG              (includes any compiler with EDG as a back-end, such as the Intel compiler)
//     Comeau           (this is a C++ to C generator)
//
// It may be useful to list the compilers/platforms the current version of 
// type_traits doesn't support:
//     Borland C++      (it simply has too many bugs with respect to templates).
//     GCC 2.96          With a little effort, type_traits can probably be made to work with this compiler.
//////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Implementation
//
// The implementation here is almost entirely based on template metaprogramming.
// This is whereby you use the compiler's template functionality to define types
// and values and make compilation decisions based on template declarations.
// Many of the algorithms here are similar to those found in books such as 
// "Modern C++ Design" and C++ libraries such as Boost. The implementations here
// are simpler and more straightforward than those found in some libraries, due
// largely to our assumption that the compiler is good at donig template programming.
///////////////////////////////////////////////////////////////////////////////



#ifndef EASTL_TYPE_TRAITS_H
#define EASTL_TYPE_TRAITS_H



#include <EASTL/internal/config.h>
#include <stddef.h>                 // Is needed for size_t usage by some traits.



namespace eastl
{

    ///////////////////////////////////////////////////////////////////////
    // integral_constant
    //
    // This is the base class for various type traits, as defined by the proposed
    // C++ standard. This is essentially a utility base class for defining properties
    // as both class constants (value) and as types (type).
    //
    template <typename T, T v>
    struct integral_constant
    {
        static const T value = v;
        typedef T value_type;
        typedef integral_constant<T, v> type;
    };


    ///////////////////////////////////////////////////////////////////////
    // true_type / false_type
    //
    // These are commonly used types in the implementation of type_traits.
    // Other integral constant types can be defined, such as those based on int.
    //
    typedef integral_constant<bool, true>  true_type;
    typedef integral_constant<bool, false> false_type;



    ///////////////////////////////////////////////////////////////////////
    // yes_type / no_type
    //
    // These are used as a utility to differentiate between two things.
    //
    typedef char yes_type;                      // sizeof(yes_type) == 1
    struct       no_type { char padding[8]; };  // sizeof(no_type)  != 1



    ///////////////////////////////////////////////////////////////////////
    // type_select
    //
    // This is used to declare a type from one of two type options. 
    // The result is based on the condition type. This has certain uses
    // in template metaprogramming.
    //
    // Example usage:
    //    typedef ChosenType = type_select<is_integral<SomeType>::value, ChoiceAType, ChoiceBType>::type;
    //
    template <bool bCondition, class ConditionIsTrueType, class ConditionIsFalseType>
    struct type_select { typedef ConditionIsTrueType type; };

    template <typename ConditionIsTrueType, class ConditionIsFalseType>
    struct type_select<false, ConditionIsTrueType, ConditionIsFalseType> { typedef ConditionIsFalseType type; };



    ///////////////////////////////////////////////////////////////////////
    // type_or
    //
    // This is a utility class for creating composite type traits.
    //
    template <bool b1, bool b2, bool b3 = false, bool b4 = false, bool b5 = false>
    struct type_or;

    template <bool b1, bool b2, bool b3, bool b4, bool b5>
    struct type_or { static const bool value = true; };

    template <> 
    struct type_or<false, false, false, false, false> { static const bool value = false; };



    ///////////////////////////////////////////////////////////////////////
    // type_and
    //
    // This is a utility class for creating composite type traits.
    //
    template <bool b1, bool b2, bool b3 = true, bool b4 = true, bool b5 = true>
    struct type_and;

    template <bool b1, bool b2, bool b3, bool b4, bool b5>
    struct type_and{ static const bool value = false; };

    template <>
    struct type_and<true, true, true, true, true>{ static const bool value = true; };



    ///////////////////////////////////////////////////////////////////////
    // type_equal
    //
    // This is a utility class for creating composite type traits.
    //
    template <int b1, int b2>
    struct type_equal{ static const bool value = (b1 == b2); };



    ///////////////////////////////////////////////////////////////////////
    // type_not_equal
    //
    // This is a utility class for creating composite type traits.
    //
    template <int b1, int b2>
    struct type_not_equal{ static const bool value = (b1 != b2); };



    ///////////////////////////////////////////////////////////////////////
    // type_not
    //
    // This is a utility class for creating composite type traits.
    //
    template <bool b>
    struct type_not{ static const bool value = true; };

    template <>
    struct type_not<true>{ static const bool value = false; };



    ///////////////////////////////////////////////////////////////////////
    // empty
    //
    template <typename T>
    struct empty{ };


} // namespace eastl


// The following files implement the type traits themselves.
#if defined(__GNUC__) && (__GNUC__ <= 2)
    #include <EASTL/internal/compat/type_fundamental.h>
    #include <EASTL/internal/compat/type_transformations.h>
    #include <EASTL/internal/compat/type_properties.h>
    #include <EASTL/internal/compat/type_compound.h>
    #include <EASTL/internal/compat/type_pod.h>
#else
    #include <EASTL/internal/type_fundamental.h>
    #include <EASTL/internal/type_transformations.h>
    #include <EASTL/internal/type_properties.h>
    #include <EASTL/internal/type_compound.h>
    #include <EASTL/internal/type_pod.h>
#endif


#endif // Header include guard





















