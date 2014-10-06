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
// EASTL/internal/type_pod.h
// Written and maintained by Paul Pedriana - 2005.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_INTERNAL_TYPE_POD_H
#define EASTL_INTERNAL_TYPE_POD_H


#include <limits.h>


namespace eastl
{


    // The following properties or relations are defined here. If the given 
    // item is missing then it simply hasn't been implemented, at least not yet.
    //    is_empty
    //    is_pod
    //    has_trivial_constructor
    //    has_trivial_copy
    //    has_trivial_assign
    //    has_trivial_destructor
    //    has_trivial_relocate       -- EA extension to the C++ standard proposal.
    //    has_nothrow_constructor
    //    has_nothrow_copy
    //    has_nothrow_assign
    //    has_virtual_destructor




    ///////////////////////////////////////////////////////////////////////
    // is_empty
    // 
    // is_empty<T>::value == true if and only if T is an empty class or struct. 
    // is_empty may only be applied to complete types.
    //
    // is_empty cannot be used with union types until is_union can be made to work.
    ///////////////////////////////////////////////////////////////////////
    template <typename T>
    struct is_empty_helper_t1 : public T { char m[64]; };
    struct is_empty_helper_t2            { char m[64]; };

    // The inheritance in empty_helper_t1 will not work with non-class types
    template <typename T, bool is_a_class = false>
    struct is_empty_helper : public false_type{};

    template <typename T>
    struct is_empty_helper<T, true> : public integral_constant<bool,
        sizeof(is_empty_helper_t1<T>) == sizeof(is_empty_helper_t2)
    >{};

    template <typename T>
    struct is_empty_helper2
    {
        typedef typename remove_cv<T>::type _T;
        typedef is_empty_helper<_T, is_class<_T>::value> type;
    };

    template <typename T> 
    struct is_empty : public is_empty_helper2<T>::type {};


    ///////////////////////////////////////////////////////////////////////
    // is_pod
    // 
    // is_pod<T>::value == true if and only if, for a given type T:
    //    - is_scalar<T>::value == true, or
    //    - T is a class or struct that has no user-defined copy 
    //      assignment operator or destructor, and T has no non-static 
    //      data members M for which is_pod<M>::value == false, and no 
    //      members of reference type, or
    //    - T is a class or struct that has no user-defined copy assignment 
    //      operator or destructor, and T has no non-static data members M for 
    //      which is_pod<M>::value == false, and no members of reference type, or
    //    - T is the type of an array of objects E for which is_pod<E>::value == true
    //
    // is_pod may only be applied to complete types.
    //
    // Without some help from the compiler or user, is_pod will not report 
    // that a struct or class is a POD, but will correctly report that 
    // built-in types such as int are PODs. The user can help the compiler
    // by using the EASTL_DECLARE_POD macro on a class.
    ///////////////////////////////////////////////////////////////////////
    template <typename T> // There's not much we can do here without some compiler extension.
    struct is_pod : public integral_constant<bool, is_void<T>::value || is_scalar<T>::value>{};

    template <typename T, size_t N>
    struct is_pod<T[N]> : public is_pod<T>{};

    template <typename T>
    struct is_POD : public is_pod<T>{};

    #define EASTL_DECLARE_POD(T) namespace eastl{ template <> struct is_pod<T> : public true_type{}; template <> struct is_pod<const T> : public true_type{}; }




    ///////////////////////////////////////////////////////////////////////
    // has_trivial_constructor
    //
    // has_trivial_constructor<T>::value == true if and only if T is a class 
    // or struct that has a trivial constructor. A constructor is trivial if
    //    - it is implicitly defined by the compiler, and
    //    - is_polymorphic<T>::value == false, and
    //    - T has no virtual base classes, and
    //    - for every direct base class of T, has_trivial_constructor<B>::value == true, 
    //      where B is the type of the base class, and
    //    - for every nonstatic data member of T that has class type or array 
    //      of class type, has_trivial_constructor<M>::value == true, 
    //      where M is the type of the data member
    //
    // has_trivial_constructor may only be applied to complete types.
    //
    // Without from the compiler or user, has_trivial_constructor will not 
    // report that a class or struct has a trivial constructor. 
    // The user can use EASTL_DECLARE_TRIVIAL_CONSTRUCTOR to help the compiler.
    //
    // A default constructor for a class X is a constructor of class X that 
    // can be called without an argument.
    ///////////////////////////////////////////////////////////////////////

    // With current compilers, this is all we can do.
    template <typename T> 
    struct has_trivial_constructor : public is_pod<T> {};

    #define EASTL_DECLARE_TRIVIAL_CONSTRUCTOR(T) namespace eastl{ template <> struct has_trivial_constructor<T> : public true_type{}; template <> struct has_trivial_constructor<const T> : public true_type{}; }




    ///////////////////////////////////////////////////////////////////////
    // has_trivial_copy
    //
    // has_trivial_copy<T>::value == true if and only if T is a class or 
    // struct that has a trivial copy constructor. A copy constructor is 
    // trivial if
    //   - it is implicitly defined by the compiler, and
    //   - is_polymorphic<T>::value == false, and
    //   - T has no virtual base classes, and
    //   - for every direct base class of T, has_trivial_copy<B>::value == true, 
    //     where B is the type of the base class, and
    //   - for every nonstatic data member of T that has class type or array 
    //     of class type, has_trivial_copy<M>::value == true, where M is the 
    //     type of the data member
    //
    // has_trivial_copy may only be applied to complete types.
    //
    // Another way of looking at this is:
    // A copy constructor for class X is trivial if it is implicitly 
    // declared and if all the following are true:
    //    - Class X has no virtual functions (10.3) and no virtual base classes (10.1).
    //    - Each direct base class of X has a trivial copy constructor.
    //    - For all the nonstatic data members of X that are of class type 
    //      (or array thereof), each such class type has a trivial copy constructor;
    //      otherwise the copy constructor is nontrivial.
    //
    // Without from the compiler or user, has_trivial_copy will not report 
    // that a class or struct has a trivial copy constructor. The user can 
    // use EASTL_DECLARE_TRIVIAL_COPY to help the compiler.
    ///////////////////////////////////////////////////////////////////////

    template <typename T> 
    struct has_trivial_copy : public integral_constant<bool, is_pod<T>::value && !is_volatile<T>::value>{};

    #define EASTL_DECLARE_TRIVIAL_COPY(T) namespace eastl{ template <> struct has_trivial_copy<T> : public true_type{}; template <> struct has_trivial_copy<const T> : public true_type{}; }


    ///////////////////////////////////////////////////////////////////////
    // has_trivial_assign
    //
    // has_trivial_assign<T>::value == true if and only if T is a class or 
    // struct that has a trivial copy assignment operator. A copy assignment 
    // operator is trivial if:
    //    - it is implicitly defined by the compiler, and
    //    - is_polymorphic<T>::value == false, and
    //    - T has no virtual base classes, and
    //    - for every direct base class of T, has_trivial_assign<B>::value == true, 
    //      where B is the type of the base class, and
    //    - for every nonstatic data member of T that has class type or array 
    //      of class type, has_trivial_assign<M>::value == true, where M is 
    //      the type of the data member.
    //
    // has_trivial_assign may only be applied to complete types.
    //
    // Without  from the compiler or user, has_trivial_assign will not 
    // report that a class or struct has trivial assignment. The user 
    // can use EASTL_DECLARE_TRIVIAL_ASSIGN to help the compiler.
    ///////////////////////////////////////////////////////////////////////

    template <typename T> 
    struct has_trivial_assign : public integral_constant<bool,
        is_pod<T>::value && !is_const<T>::value && !is_volatile<T>::value
    >{};

    #define EASTL_DECLARE_TRIVIAL_ASSIGN(T) namespace eastl{ template <> struct has_trivial_assign<T> : public true_type{}; template <> struct has_trivial_assign<const T> : public true_type{}; }




    ///////////////////////////////////////////////////////////////////////
    // has_trivial_destructor
    //
    // has_trivial_destructor<T>::value == true if and only if T is a class 
    // or struct that has a trivial destructor. A destructor is trivial if
    //    - it is implicitly defined by the compiler, and
    //    - for every direct base class of T, has_trivial_destructor<B>::value == true, 
    //      where B is the type of the base class, and
    //    - for every nonstatic data member of T that has class type or 
    //      array of class type, has_trivial_destructor<M>::value == true, 
    //      where M is the type of the data member
    //
    // has_trivial_destructor may only be applied to complete types.
    //
    // Without from the compiler or user, has_trivial_destructor will not 
    // report that a class or struct has a trivial destructor. 
    // The user can use EASTL_DECLARE_TRIVIAL_DESTRUCTOR to help the compiler.
    ///////////////////////////////////////////////////////////////////////
    
    // With current compilers, this is all we can do.
    template <typename T> 
    struct has_trivial_destructor : public is_pod<T>{};

    #define EASTL_DECLARE_TRIVIAL_DESTRUCTOR(T) namespace eastl{ template <> struct has_trivial_destructor<T> : public true_type{}; template <> struct has_trivial_destructor<const T> : public true_type{}; }


    ///////////////////////////////////////////////////////////////////////
    // has_trivial_relocate
    //
    // This is an EA extension to the type traits standard.
    //
    // A trivially relocatable object is one that can be safely memmove'd 
    // to uninitialized memory. construction, assignment, and destruction 
    // properties are not addressed by this trait. A type that has the 
    // is_fundamental trait would always have the has_trivial_relocate trait. 
    // A type that has the has_trivial_constructor, has_trivial_copy or 
    // has_trivial_assign traits would usally have the has_trivial_relocate 
    // trait, but this is not strictly guaranteed.
    //
    // The user can use EASTL_DECLARE_TRIVIAL_RELOCATE to help the compiler.
    ///////////////////////////////////////////////////////////////////////

    // With current compilers, this is all we can do.
    template <typename T> 
    struct has_trivial_relocate : public integral_constant<bool, is_pod<T>::value && !is_volatile<T>::value>{};

    #define EASTL_DECLARE_TRIVIAL_RELOCATE(T) namespace eastl{ template <> struct has_trivial_relocate<T> : public true_type{}; template <> struct has_trivial_relocate<const T> : public true_type{}; }


} // namespace eastl


#endif // Header include guard






















