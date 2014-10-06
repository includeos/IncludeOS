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
// EASTL/algorithm.h
//
// Written and maintained by Paul Pedriana - 2005.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements some of the primary algorithms from the C++ STL 
// algorithm library. These versions are just like that STL versions and so 
// are redundant. They are provided solely for the purpose of projects that
// either cannot use standard C++ STL or want algorithms that have guaranteed
// identical behaviour across platforms.
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Definitions
// 
// You will notice that we are very particular about the templated typenames
// we use here. You will notice that we follow the C++ standard closely in 
// these respects. Each of these typenames have a specific meaning; 
// this is why we don't just label templated arguments with just letters 
// such as T, U, V, A, B. Here we provide a quick reference for the typenames
// we use. See the C++ standard, section 25-8 for more details.
//    --------------------------------------------------------------
//    typename                     Meaning
//    --------------------------------------------------------------
//    T                            The value type.
//    Compare                      A function which takes two arguments and returns the lesser of the two.
//    Predicate                    A function which takes one argument returns true if the argument meets some criteria.
//    BinaryPredicate              A function which takes two arguments and returns true if some criteria is met (e.g. they are equal).
//    StrickWeakOrdering           A BinaryPredicate that compares two objects, returning true if the first precedes the second. Like Compare but has additional requirements. Used for sorting routines.
//    Function                     A function which takes one argument and applies some operation to the target.
//    Size                         A count or size.
//    Generator                    A function which takes no arguments and returns a value (which will usually be assigned to an object).
//    UnaryOperation               A function which takes one argument and returns a value (which will usually be assigned to second object).
//    BinaryOperation              A function which takes two arguments and returns a value (which will usually be assigned to a third object).
//    InputIterator                An input iterator (iterator you read from) which allows reading each element only once and only in a forward direction.
//    ForwardIterator              An input iterator which is like InputIterator except it can be reset back to the beginning.
//    BidirectionalIterator        An input iterator which is like ForwardIterator except it can be read in a backward direction as well.
//    RandomAccessIterator         An input iterator which can be addressed like an array. It is a superset of all other input iterators.
//    OutputIterator               An output iterator (iterator you write to) which allows writing each element only once in only in a forward direction.
//
// Note that with iterators that a function which takes an InputIterator will 
// also work with a ForwardIterator, BidirectionalIterator, or RandomAccessIterator.
// The given iterator type is merely the -minimum- supported functionality the 
// iterator must support.
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Optimizations
//
// There are a number of opportunities for opptimizations that we take here
// in this library. The most obvious kinds are those that subsitute memcpy
// in the place of a conventional loop for data types with which this is 
// possible. The algorithms here are optimized to a higher level than currently
// available C++ STL algorithms from vendors. This is especially
// so for game programming on console devices, as we do things such as reduce 
// branching relative to other STL algorithm implementations. However, the 
// proper implementation of these algorithm optimizations is a fairly tricky
// thing. 
//
// The various things we look to take advantage of in order to implement 
// optimizations include:
//    - Taking advantage of random access iterators.
//    - Taking advantage of POD (plain old data) data types.
//    - Taking advantage of type_traits in general.
//    - Reducing branching and taking advantage of likely branch predictions.
//    - Taking advantage of issues related to pointer and reference aliasing.
//    - Improving cache coherency during memory accesses.
//    - Making code more likely to be inlinable by the compiler.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_ALGORITHM_H
#define EASTL_ALGORITHM_H


#include <EASTL/internal/config.h>
#include <EASTL/utility.h>
#include <EASTL/iterator.h>
#include <EASTL/functional.h>
#include <EASTL/internal/generic_iterator.h>
#include <EASTL/type_traits.h>

#ifdef _MSC_VER
    #pragma warning(push, 0)
#endif
#include <stddef.h>
#ifdef __MWERKS__
    #include <../Include/string.h> // Force the compiler to use the std lib header.
#else
    #include <string.h> // memcpy, memcmp, memmove
#endif
#ifdef _MSC_VER
    #pragma warning(pop)
#endif


///////////////////////////////////////////////////////////////////////////////
// min/max workaround
//
// MSVC++ has #defines for min/max which collide with the min/max algorithm
// declarations. The following may still not completely resolve some kinds of
// problems with MSVC++ #defines, though it deals with most cases in production
// game code.
//
#if EASTL_NOMINMAX
    #ifdef min
        #undef min
    #endif
    #ifdef max
        #undef max
    #endif
#endif




namespace eastl
{
    #if EASTL_MINMAX_ENABLED

        /// min
        ///
        /// Min returns the lesser of its two arguments; it returns the first 
        /// argument if neither is less than the other. The two arguments are
        /// compared with operator <.
        ///
        /// This min and our other min implementations are defined as returning:
        ///     b < a ? b : a
        /// which for example may in practice result in something different than:
        ///     b <= a ? b : a
        /// in the case where b is different from a (though they compare as equal).
        /// We choose the specific ordering here because that's the ordering 
        /// done by other STL implementations.
        ///
        template <typename T>
        inline const T&
        min(const T& a, const T& b)
        {
            return b < a ? b : a;
        }
    #endif // EASTL_MINMAX_ENABLED


    /// min_alt
    ///
    /// This is an alternative version of min that avoids any possible 
    /// collisions with Microsoft #defines of min and max.
    ///
    /// See min(a, b) for detailed specifications.
    ///
    template <typename T>
    inline const T&
    min_alt(const T& a, const T& b)
    {
        return b < a ? b : a;
    }

    #if EASTL_MINMAX_ENABLED
        /// min
        ///
        /// Min returns the lesser of its two arguments; it returns the first 
        /// argument if neither is less than the other. The two arguments are
        /// compared with the Compare function (or function object), which 
        /// takes two arguments and returns true if the first is less than 
        /// the second.
        ///
        /// See min(a, b) for detailed specifications.
        ///
        /// Example usage:
        ///    struct A{ int a; };
        ///    struct Struct{ bool operator()(const A& a1, const A& a2){ return a1.a < a2.a; } };
        ///
        ///    A a1, a2, a3;
        ///    a3 = min(a1, a2, Struct());
        ///
        /// Example usage:
        ///    struct B{ int b; };
        ///    inline bool Function(const B& b1, const B& b2){ return b1.b < b2.b; }
        ///
        ///    B b1, b2, b3;
        ///    b3 = min(b1, b2, Function);
        ///
        template <typename T, typename Compare>
        inline const T&
        min(const T& a, const T& b, Compare compare)
        {
            return compare(b, a) ? b : a;
        }

    #endif // EASTL_MINMAX_ENABLED


    /// min_alt
    ///
    /// This is an alternative version of min that avoids any possible 
    /// collisions with Microsoft #defines of min and max.
    ///
    /// See min(a, b) for detailed specifications.
    ///
    template <typename T, typename Compare>
    inline const T&
    min_alt(const T& a, const T& b, Compare compare)
    {
        return compare(b, a) ? b : a;
    }


    #if EASTL_MINMAX_ENABLED
        /// max
        ///
        /// Max returns the greater of its two arguments; it returns the first 
        /// argument if neither is greater than the other. The two arguments are
        /// compared with operator < (and not operator >).
        ///
        /// This min and our other min implementations are defined as returning:
        ///     a < b ? b : a
        /// which for example may in practice result in something different than:
        ///     a <= b ? b : a
        /// in the case where b is different from a (though they compare as equal).
        /// We choose the specific ordering here because that's the ordering 
        /// done by other STL implementations.
        ///
        template <typename T>
        inline const T&
        max(const T& a, const T& b)
        {
            return a < b ? b : a;
        }
    #endif // EASTL_MINMAX_ENABLED


    /// max_alt
    ///
    /// This is an alternative version of max that avoids any possible 
    /// collisions with Microsoft #defines of min and max.
    ///
    template <typename T>
    inline const T&
    max_alt(const T& a, const T& b)
    {
        return a < b ? b : a;
    }

    #if EASTL_MINMAX_ENABLED
        /// max
        ///
        /// Min returns the lesser of its two arguments; it returns the first 
        /// argument if neither is less than the other. The two arguments are
        /// compared with the Compare function (or function object), which 
        /// takes two arguments and returns true if the first is less than 
        /// the second.
        ///
        template <typename T, typename Compare>
        inline const T&
        max(const T& a, const T& b, Compare compare)
        {
            return compare(a, b) ? b : a;
        }

    #endif
 

    /// max_alt
    ///
    /// This is an alternative version of max that avoids any possible 
    /// collisions with Microsoft #defines of min and max.
    ///
    template <typename T, typename Compare>
    inline const T&
    max_alt(const T& a, const T& b, Compare compare)
    {
        return compare(a, b) ? b : a;
    }



    /// min_element
    ///
    /// min_element finds the smallest element in the range [first, last). 
    /// It returns the first iterator i in [first, last) such that no other 
    /// iterator in [first, last) points to a value smaller than *i. 
    /// The return value is last if and only if [first, last) is an empty range.
    ///
    /// Returns: The first iterator i in the range [first, last) such that 
    /// for any iterator j in the range [first, last) the following corresponding 
    /// condition holds: !(*j < *i).
    ///
    /// Complexity: Exactly 'max((last - first) - 1, 0)' applications of the 
    /// corresponding comparisons.
    ///
    template <typename ForwardIterator>
    ForwardIterator min_element(ForwardIterator first, ForwardIterator last)
    {
        if(first != last)
        {
            ForwardIterator currentMin = first;

            while(++first != last)
            {
                if(*first < *currentMin)
                    currentMin = first;
            }
            return currentMin;
        }
        return first;
    }


    /// min_element
    ///
    /// min_element finds the smallest element in the range [first, last). 
    /// It returns the first iterator i in [first, last) such that no other 
    /// iterator in [first, last) points to a value smaller than *i. 
    /// The return value is last if and only if [first, last) is an empty range.
    ///
    /// Returns: The first iterator i in the range [first, last) such that 
    /// for any iterator j in the range [first, last) the following corresponding 
    /// conditions hold: compare(*j, *i) == false.
    ///
    /// Complexity: Exactly 'max((last - first) - 1, 0)' applications of the 
    /// corresponding comparisons.
    ///
    template <typename ForwardIterator, typename Compare>
    ForwardIterator min_element(ForwardIterator first, ForwardIterator last, Compare compare)
    {
        if(first != last)
        {
            ForwardIterator currentMin = first;

            while(++first != last)
            {
                if(compare(*first, *currentMin))
                    currentMin = first;
            }
            return currentMin;
        }
        return first;
    }


    /// max_element
    ///
    /// max_element finds the largest element in the range [first, last). 
    /// It returns the first iterator i in [first, last) such that no other 
    /// iterator in [first, last) points to a value greater than *i. 
    /// The return value is last if and only if [first, last) is an empty range.
    ///
    /// Returns: The first iterator i in the range [first, last) such that 
    /// for any iterator j in the range [first, last) the following corresponding 
    /// condition holds: !(*i < *j).
    ///
    /// Complexity: Exactly 'max((last - first) - 1, 0)' applications of the 
    /// corresponding comparisons.
    ///
    template <typename ForwardIterator>
    ForwardIterator max_element(ForwardIterator first, ForwardIterator last)
    {
        if(first != last)
        {
            ForwardIterator currentMax = first;

            while(++first != last)
            {
                if(*currentMax < *first)
                    currentMax = first;
            }
            return currentMax;
        }
        return first;
    }


    /// max_element
    ///
    /// max_element finds the largest element in the range [first, last). 
    /// It returns the first iterator i in [first, last) such that no other 
    /// iterator in [first, last) points to a value greater than *i. 
    /// The return value is last if and only if [first, last) is an empty range.
    ///
    /// Returns: The first iterator i in the range [first, last) such that 
    /// for any iterator j in the range [first, last) the following corresponding 
    /// condition holds: compare(*i, *j) == false.
    ///
    /// Complexity: Exactly 'max((last - first) - 1, 0)' applications of the 
    /// corresponding comparisons.
    ///
    template <typename ForwardIterator, typename Compare>
    ForwardIterator max_element(ForwardIterator first, ForwardIterator last, Compare compare)
    {
        if(first != last)
        {
            ForwardIterator currentMax = first;

            while(++first != last)
            {
                if(compare(*currentMax, *first))
                    currentMax = first;
            }
            return currentMax;
        }
        return first;
    }


    /// median
    ///
    /// median finds which element of three (a, b, d) is in-between the other two.
    /// If two or more elements are equal, the first (e.g. a before b) is chosen.
    ///
    /// Complexity: Either two or three comparisons will be required, depending 
    /// on the values.
    ///
    template <typename T>
    inline const T& median(const T& a, const T& b, const T& c)
    {
        if(a < b)
        {
            if(b < c)
                return b;
            else if(a < c)
                return c;
            else
                return a;
        }
        else if(a < c)
            return a;
        else if(b < c)
            return c;
        return b;
    }


    /// median
    ///
    /// median finds which element of three (a, b, d) is in-between the other two.
    /// If two or more elements are equal, the first (e.g. a before b) is chosen.
    ///
    /// Complexity: Either two or three comparisons will be required, depending 
    /// on the values.
    ///
    template <typename T, typename Compare>
    inline const T& median(const T& a, const T& b, const T& c, Compare compare)
    {
        if(compare(a, b))
        {
            if(compare(b, c))
                return b;
            else if(compare(a, c))
                return c;
            else
                return a;
        }
        else if(compare(a, c))
            return a;
        else if(compare(b, c))
            return c;
        return b;
    }



    /// swap
    ///
    /// Assigns the contents of a to b and the contents of b to a. 
    /// A temporary instance of type T is created and destroyed
    /// in the process.
    ///
    /// This function is used by numerous other algorithms, and as 
    /// such it may in some cases be feasible and useful for the user 
    /// to implement an override version of this function which is 
    /// more efficient in some way. 
    ///
    template <typename T>
    inline void swap(T& a, T& b)
    {
#ifdef EA_COMPILER_HAS_MOVE_SEMANTICS
        T temp = std::move(a);
        a = std::move(b);
        b = std::move(temp);
#else
        T temp(a);
        a = b;
        b = temp;
#endif
    }



    // iter_swap helper functions
    //
    template <bool bTypesAreEqual>
    struct iter_swap_impl
    {
        template <typename ForwardIterator1, typename ForwardIterator2>
        static void iter_swap(ForwardIterator1 a, ForwardIterator2 b)
        {
            typedef typename eastl::iterator_traits<ForwardIterator1>::value_type value_type_a;

            value_type_a temp(*a);
            *a = *b;
            *b = temp; 
        }
    };

    template <>
    struct iter_swap_impl<true>
    {
        template <typename ForwardIterator1, typename ForwardIterator2>
        static void iter_swap(ForwardIterator1 a, ForwardIterator2 b)
        {
            eastl::swap(*a, *b);
        }
    };

    /// iter_swap
    ///
    /// Equivalent to swap(*a, *b), though the user can provide an override to
    /// iter_swap that is independent of an override which may exist for swap.
    ///
    /// We provide a version of iter_swap which uses swap when the swapped types 
    /// are equal but a manual implementation otherwise. We do this because the 
    /// C++ standard defect report says that iter_swap(a, b) must be implemented 
    /// as swap(*a, *b) when possible.
    ///
    template <typename ForwardIterator1, typename ForwardIterator2>
    inline void iter_swap(ForwardIterator1 a, ForwardIterator2 b)
    {
        typedef typename eastl::iterator_traits<ForwardIterator1>::value_type value_type_a;
        typedef typename eastl::iterator_traits<ForwardIterator2>::value_type value_type_b;
        typedef typename eastl::iterator_traits<ForwardIterator1>::reference  reference_a;
        typedef typename eastl::iterator_traits<ForwardIterator2>::reference  reference_b;

        iter_swap_impl<type_and<is_same<value_type_a, value_type_b>::value, is_same<value_type_a&, reference_a>::value, is_same<value_type_b&, reference_b>::value >::value >::iter_swap(a, b);
    }



    /// swap_ranges
    ///
    /// Swaps each of the elements in the range [first1, last1) with the 
    /// corresponding element in the range [first2, first2 + (last1 - first1)). 
    ///
    /// Effects: For each nonnegative integer n < (last1 - first1),
    /// performs: swap(*(first1 + n), *(first2 + n)).
    ///
    /// Requires: The two ranges [first1, last1) and [first2, first2 + (last1 - first1))
    /// shall not overlap.
    ///
    /// Returns: first2 + (last1 - first1). That is, returns the end of the second range.
    ///
    /// Complexity: Exactly 'last1 - first1' swaps.
    ///
    template <typename ForwardIterator1, typename ForwardIterator2>
    inline ForwardIterator2
    swap_ranges(ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2)
    {
        for(; first1 != last1; ++first1, ++first2)
            iter_swap(first1, first2);
        return first2;
    }



    /// adjacent_find
    ///
    /// Returns: The first iterator i such that both i and i + 1 are in the range 
    /// [first, last) for which the following corresponding conditions hold: *i == *(i + 1). 
    /// Returns last if no such iterator is found.
    ///
    /// Complexity: Exactly 'find(first, last, value) - first' applications of the corresponding predicate.
    ///
    template <typename ForwardIterator>
    inline ForwardIterator 
    adjacent_find(ForwardIterator first, ForwardIterator last)
    {
        if(first != last)
        {
            ForwardIterator i = first;

            for(++i; i != last; ++i)
            {
                if(*first == *i)
                    return first;
                first = i;
            }
        }
        return last;
    }



    /// adjacent_find
    ///
    /// Returns: The first iterator i such that both i and i + 1 are in the range 
    /// [first, last) for which the following corresponding conditions hold: predicate(*i, *(i + 1)) != false. 
    /// Returns last if no such iterator is found.
    ///
    /// Complexity: Exactly 'find(first, last, value) - first' applications of the corresponding predicate.
    ///
    template <typename ForwardIterator, typename BinaryPredicate>
    inline ForwardIterator 
    adjacent_find(ForwardIterator first, ForwardIterator last, BinaryPredicate predicate)
    {
        if(first != last)
        {
            ForwardIterator i = first;

            for(++i; i != last; ++i)
            {
                if(predicate(*first, *i))
                    return first;
                first = i;
            }
        }
        return last;
    }




    // copy
    //
    // We implement copy via some helper functions whose purpose is to 
    // try to use memcpy when possible. We need to use type_traits and 
    // iterator categories to do this.
    //
    template <bool bHasTrivialCopy, typename IteratorTag>
    struct copy_impl
    {
        template <typename InputIterator, typename OutputIterator>
        static OutputIterator do_copy(InputIterator first, InputIterator last, OutputIterator result)
        {
            for(; first != last; ++result, ++first)
                *result = *first;
            return result;
        }
    };

    template <>
    struct copy_impl<true, EASTL_ITC_NS::random_access_iterator_tag> // If we have a trivally copyable random access array, use memcpy
    {
        template <typename T>
        static T* do_copy(const T* first, const T* last, T* result)
        {
            // We cannot use memcpy because memcpy requires the entire source and dest ranges to be 
            // non-overlapping, whereas the copy algorithm requires only that 'result' not be within
            // the range from first to last.
            return (T*)memmove(result, first, (size_t)((uintptr_t)last - (uintptr_t)first)) + (last - first);
        }
    };

    // copy_chooser
    // Calls one of the above copy_impl functions.
    template <typename InputIterator, typename OutputIterator>
    inline OutputIterator
    copy_chooser(InputIterator first, InputIterator last, OutputIterator result)
    {
        typedef typename eastl::iterator_traits<InputIterator>::iterator_category IC;
        typedef typename eastl::iterator_traits<InputIterator>::value_type        value_type_input;
        typedef typename eastl::iterator_traits<OutputIterator>::value_type       value_type_output;

        const bool bHasTrivialCopy = type_and<has_trivial_assign<value_type_input>::value, 
                                              is_pointer<InputIterator>::value,
                                              is_pointer<OutputIterator>::value,
                                              is_same<value_type_input, value_type_output>::value>::value;

        return eastl::copy_impl<bHasTrivialCopy, IC>::do_copy(first, last, result);
    }

    // copy_generic_iterator
    // Converts a copy call via a generic_iterator to a copy call via the iterator type the 
    // generic_iterator holds. We do this because generic_iterator's purpose is to hold
    // iterators that are simply pointers, and if we want the functions above to be fast,
    // we need them to see the pointers and not an iterator that wraps the pointers as 
    // does generic_iterator. We are forced into using a templated struct with a templated
    // do_copy member function because C++ doesn't allow specializations for standalone functions.
    template <bool bInputIsGenericIterator, bool bOutputIsGenericIterator>
    struct copy_generic_iterator
    {
        template <typename InputIterator, typename OutputIterator>
        static OutputIterator do_copy(InputIterator first, InputIterator last, OutputIterator result)
        {
            return eastl::copy_chooser(first, last, result);
        }
    };

    template <>
    struct copy_generic_iterator<true, false>
    {
        template <typename InputIterator, typename OutputIterator>
        static OutputIterator do_copy(InputIterator first, InputIterator last, OutputIterator result)
        {
            return eastl::copy_chooser(first.base(), last.base(), result); // first.base(), last.base() will resolve to a pointer (e.g. T*).
        }
    };

    template <>
    struct copy_generic_iterator<false, true>
    {
        template <typename InputIterator, typename OutputIterator>
        static OutputIterator do_copy(InputIterator first, InputIterator last, OutputIterator result)
        {
            return OutputIterator(eastl::copy_chooser(first, last, result.base())); // Have to convert to OutputIterator because result.base() is a T*
        }
    };

    template <>
    struct copy_generic_iterator<true, true>
    {
        template <typename InputIterator, typename OutputIterator>
        static OutputIterator do_copy(InputIterator first, InputIterator last, OutputIterator result)
        {
            return OutputIterator(eastl::copy_chooser(first.base(), last.base(), result.base())); // Have to convert to OutputIterator because result.base() is a T*
        }
    };

    /// copy
    ///
    /// Effects: Copies elements in the range [first, last) into the range [result, result + (last - first))
    /// starting from first and proceeding to last. For each nonnegative integer n < (last - first),
    /// performs *(result + n) = *(first + n).
    ///
    /// Returns: result + (last - first). That is, returns the end of the result. Note that this 
    /// is different from how memcpy works, as memcpy returns the beginning of the result.
    ///
    /// Requires: result shall not be in the range [first, last).
    ///
    /// Complexity: Exactly 'last - first' assignments.
    ///
    /// Note: This function is like memcpy in that the result must not be with the 
    /// range of (first, last), as would cause memory to be overwritten incorrectly.
    ///
    template <typename InputIterator, typename OutputIterator>
    inline OutputIterator
    copy(InputIterator first, InputIterator last, OutputIterator result)
    {
        //#ifdef __GNUC__ // GCC has template depth problems and this shortcut may need to be enabled.
        //    return eastl::copy_chooser(first, last, result);
        //#else
            const bool bInputIsGenericIterator  = is_generic_iterator<InputIterator>::value;
            const bool bOutputIsGenericIterator = is_generic_iterator<OutputIterator>::value;
            return eastl::copy_generic_iterator<bInputIsGenericIterator, bOutputIsGenericIterator>::do_copy(first, last, result);
        //#endif
    }




    // copy_backward
    //
    // We implement copy_backward via some helper functions whose purpose is 
    // to try to use memcpy when possible. We need to use type_traits and 
    // iterator categories to do this.
    //
    template <bool bHasTrivialCopy, typename IteratorTag>
    struct copy_backward_impl
    {
        template <typename BidirectionalIterator1, typename BidirectionalIterator2>
        static BidirectionalIterator2 do_copy(BidirectionalIterator1 first, BidirectionalIterator1 last, BidirectionalIterator2 result)
        {
            while(last != first)
                *--result = *--last;
            return result;
        }
    };

    template <>
    struct copy_backward_impl<true, EASTL_ITC_NS::random_access_iterator_tag> // If we have a trivally copyable random access array, use memcpy
    {
        template <typename T>
        static T* do_copy(const T* first, const T* last, T* result)
        {
            return (T*)memmove(result - (last - first), first, (size_t)((uintptr_t)last - (uintptr_t)first));
        }
    };

    // copy_backward_chooser
    // Calls one of the above copy_backward_impl functions.
    template <typename InputIterator, typename OutputIterator>
    inline OutputIterator
    copy_backward_chooser(InputIterator first, InputIterator last, OutputIterator result)
    {
        typedef typename eastl::iterator_traits<InputIterator>::iterator_category IC;
        typedef typename eastl::iterator_traits<InputIterator>::value_type        value_type_input;
        typedef typename eastl::iterator_traits<OutputIterator>::value_type       value_type_output;

        const bool bHasTrivialCopy = type_and<has_trivial_assign<value_type_input>::value, 
                                              is_pointer<InputIterator>::value,
                                              is_pointer<OutputIterator>::value,
                                              is_same<value_type_input, value_type_output>::value>::value;

        return eastl::copy_backward_impl<bHasTrivialCopy, IC>::do_copy(first, last, result);
    }

    // copy_backward_generic_iterator
    // Converts a copy call via a generic_iterator to a copy call via the iterator type the 
    // generic_iterator holds. We do this because generic_iterator's purpose is to hold
    // iterators that are simply pointers, and if we want the functions above to be fast,
    // we need them to see the pointers and not an iterator that wraps the pointers as 
    // does generic_iterator. We are forced into using a templated struct with a templated
    // do_copy member function because C++ doesn't allow specializations for standalone functions.
    template <bool bInputIsGenericIterator, bool bOutputIsGenericIterator>
    struct copy_backward_generic_iterator
    {
        template <typename InputIterator, typename OutputIterator>
        static OutputIterator do_copy(InputIterator first, InputIterator last, OutputIterator result)
        {
            return eastl::copy_backward_chooser(first, last, result);
        }
    };

    template <>
    struct copy_backward_generic_iterator<true, false>
    {
        template <typename InputIterator, typename OutputIterator>
        static OutputIterator do_copy(InputIterator first, InputIterator last, OutputIterator result)
        {
            return eastl::copy_backward_chooser(first.base(), last.base(), result); // first.base(), last.base() will resolve to a pointer (e.g. T*).
        }
    };

    template <>
    struct copy_backward_generic_iterator<false, true>
    {
        template <typename InputIterator, typename OutputIterator>
        static OutputIterator do_copy(InputIterator first, InputIterator last, OutputIterator result)
        {
            return OutputIterator(eastl::copy_backward_chooser(first, last, result.base())); // Have to convert to OutputIterator because result.base() is a T*
        }
    };

    template <>
    struct copy_backward_generic_iterator<true, true>
    {
        template <typename InputIterator, typename OutputIterator>
        static OutputIterator do_copy(InputIterator first, InputIterator last, OutputIterator result)
        {
            return OutputIterator(eastl::copy_backward_chooser(first.base(), last.base(), result.base())); // Have to convert to OutputIterator because result.base() is a T*
        }
    };

    /// copy_backward
    ///
    /// copies memory in the range of [first, last) to the range *ending* with result.
    /// 
    /// Effects: Copies elements in the range [first, last) into the range 
    /// [result - (last - first), result) starting from last 1 and proceeding to first. 
    /// For each positive integer n <= (last - first), performs *(result n) = *(last - n).
    ///
    /// Requires: result shall not be in the range [first, last).
    ///
    /// Returns: result - (last - first). That is, returns the beginning of the result range.
    ///
    /// Complexity: Exactly 'last - first' assignments.
    /// 
    template <typename BidirectionalIterator1, typename BidirectionalIterator2>
    inline BidirectionalIterator2
    copy_backward(BidirectionalIterator1 first, BidirectionalIterator1 last, BidirectionalIterator2 result)
    {
        const bool bInputIsGenericIterator  = is_generic_iterator<BidirectionalIterator1>::value;
        const bool bOutputIsGenericIterator = is_generic_iterator<BidirectionalIterator2>::value;

        return eastl::copy_backward_generic_iterator<bInputIsGenericIterator, bOutputIsGenericIterator>::do_copy(first, last, result);
    }



    /// count
    ///
    /// Counts the number of items in the range of [first, last) which equal the input value.
    ///
    /// Effects: Returns the number of iterators i in the range [first, last) for which the 
    /// following corresponding conditions hold: *i == value.
    ///
    /// Complexity: At most 'last - first' applications of the corresponding predicate.
    ///
    /// Note: The predicate version of count is count_if and not another variation of count.
    /// This is because both versions would have three parameters and there could be ambiguity.
    ///
    template <typename InputIterator, typename T>
    inline typename eastl::iterator_traits<InputIterator>::difference_type
    count(InputIterator first, InputIterator last, const T& value)
    {
        typename eastl::iterator_traits<InputIterator>::difference_type result = 0;

        for(; first != last; ++first)
        {
            if(*first == value)
                ++result;
        }
        return result;
    }


    /// count_if
    ///
    /// Counts the number of items in the range of [first, last) which match 
    /// the input value as defined by the input predicate function.
    ///
    /// Effects: Returns the number of iterators i in the range [first, last) for which the 
    /// following corresponding conditions hold: predicate(*i) != false.
    ///
    /// Complexity: At most 'last - first' applications of the corresponding predicate.
    ///
    /// Note: The non-predicate version of count_if is count and not another variation of count_if.
    /// This is because both versions would have three parameters and there could be ambiguity.
    ///
    template <typename InputIterator, typename Predicate>
    inline typename eastl::iterator_traits<InputIterator>::difference_type
    count_if(InputIterator first, InputIterator last, Predicate predicate)
    {
        typename eastl::iterator_traits<InputIterator>::difference_type result = 0;

        for(; first != last; ++first)
        {
            if(predicate(*first))
                ++result;
        }
        return result;
    }



    // fill
    //
    // We implement some fill helper functions in order to allow us to optimize it
    // where possible.
    //
    template <bool bIsScalar>
    struct fill_imp
    {
        template <typename ForwardIterator, typename T>
        static void do_fill(ForwardIterator first, ForwardIterator last, const T& value)
        {
            // The C++ standard doesn't specify whether we need to create a temporary
            // or not, but all std STL implementations are written like what we have here.
            for(; first != last; ++first)
                *first = value;
        }
    };

    template <>
    struct fill_imp<true>
    {
        template <typename ForwardIterator, typename T>
        static void do_fill(ForwardIterator first, ForwardIterator last, const T& value)
        {
            // We create a temp and fill from that because value might alias to the 
            // destination range and so the compiler would be forced into generating 
            // less efficient code.
            for(const T temp(value); first != last; ++first)
                *first = temp;
        }
    };

    /// fill
    ///
    /// fill is like memset in that it assigns a single value repeatedly to a 
    /// destination range. It allows for any type of iterator (not just an array)
    /// and the source value can be any type, not just a byte.
    /// Note that the source value (which is a reference) can come from within 
    /// the destination range.
    ///
    /// Effects: Assigns value through all the iterators in the range [first, last).
    ///
    /// Complexity: Exactly 'last - first' assignments.
    ///
    /// Note: The C++ standard doesn't specify anything about the value parameter
    /// coming from within the first-last range. All std STL implementations act
    /// as if the standard specifies that value must not come from within this range.
    ///
    template <typename ForwardIterator, typename T>
    inline void fill(ForwardIterator first, ForwardIterator last, const T& value)
    {
        eastl::fill_imp< is_scalar<T>::value >::do_fill(first, last, value);

        // Possibly better implementation, as it will deal with small PODs as well as scalars:
        // bEasyCopy is true if the type has a trivial constructor (e.g. is a POD) and if 
        // it is small. Thus any built-in type or any small user-defined struct will qualify.
        //const bool bEasyCopy = eastl::type_and<eastl::has_trivial_constructor<T>::value, 
        //                                       eastl::integral_constant<bool, (sizeof(T) <= 16)>::value;
        //eastl::fill_imp<bEasyCopy>::do_fill(first, last, value);

    }

    inline void fill(char* first, char* last, const char& c) // It's debateable whether we should use 'char& c' or 'char c' here.
    {
        memset(first, (unsigned char)c, (size_t)(last - first));
    }

    inline void fill(char* first, char* last, const int c) // This is used for cases like 'fill(first, last, 0)'.
    {
        memset(first, (unsigned char)c, (size_t)(last - first));
    }

    inline void fill(unsigned char* first, unsigned char* last, const unsigned char& c)
    {
        memset(first, (unsigned char)c, (size_t)(last - first));
    }

    inline void fill(unsigned char* first, unsigned char* last, const int c)
    {
        memset(first, (unsigned char)c, (size_t)(last - first));
    }

    inline void fill(signed char* first, signed char* last, const signed char& c)
    {
        memset(first, (unsigned char)c, (size_t)(last - first));
    }

    inline void fill(signed char* first, signed char* last, const int c)
    {
        memset(first, (unsigned char)c, (size_t)(last - first));
    }

    #if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__SNC__) || defined(__ICL) || defined(__PPU__) || defined(__SPU__) // SN = SN compiler, ICL = Intel compiler, PPU == PS3 processor, SPU = PS3 cell processor
        inline void fill(bool* first, bool* last, const bool& b)
        {
            memset(first, (char)b, (size_t)(last - first));
        }
    #endif




    // fill_n
    //
    // We implement some fill helper functions in order to allow us to optimize it
    // where possible.
    //
    template <bool bIsScalar>
    struct fill_n_imp
    {
        template <typename OutputIterator, typename Size, typename T>
        static OutputIterator do_fill(OutputIterator first, Size n, const T& value)
        {
            for(; n-- > 0; ++first)
                *first = value;
            return first;
        }
    };

    template <>
    struct fill_n_imp<true>
    {
        template <typename OutputIterator, typename Size, typename T>
        static OutputIterator do_fill(OutputIterator first, Size n, const T& value)
        {
            // We create a temp and fill from that because value might alias to 
            // the destination range and so the compiler would be forced into 
            // generating less efficient code.
            for(const T temp = value; n-- > 0; ++first)
                *first = temp;
            return first;
        }
    };

    /// fill_n
    ///
    /// The fill_n function is very much like memset in that a copies a source value
    /// n times into a destination range. The source value may come from within 
    /// the destination range.
    ///
    /// Effects: Assigns value through all the iterators in the range [first, first + n).
    ///
    /// Complexity: Exactly n assignments.
    ///
    template <typename OutputIterator, typename Size, typename T>
    OutputIterator fill_n(OutputIterator first, Size n, const T& value)
    {
        #ifdef _MSC_VER // VC++ up to and including VC8 blow up when you pass a 64 bit scalar to the do_fill function.
            return eastl::fill_n_imp< is_scalar<T>::value && (sizeof(T) <= sizeof(uint32_t)) >::do_fill(first, n, value);
        #else
            return eastl::fill_n_imp< is_scalar<T>::value >::do_fill(first, n, value);
        #endif
    }

    template <typename Size>
    inline char* fill_n(char* first, Size n, const char& c)
    {
        return (char*)memset(first, (char)c, (size_t)n) + n;
    }

    template <typename Size>
    inline unsigned char* fill_n(unsigned char* first, Size n, const unsigned char& c)
    {
        return (unsigned char*)memset(first, (unsigned char)c, (size_t)n) + n;
    }

    template <typename Size>
    inline signed char* fill_n(signed char* first, Size n, const signed char& c)
    {
        return (signed char*)memset(first, (signed char)c, n) + (size_t)n;
    }

    #if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__SNC__) || defined(__ICL) || defined(__PPU__) || defined(__SPU__) // SN = SN compiler, ICL = Intel compiler, PU == PS3 processor, SPU = PS3 cell processor
        template <typename Size>
        inline bool* fill_n(bool* first, Size n, const bool& b)
        {
            return (bool*)memset(first, (char)b, n) + (size_t)n;
        }
    #endif



    /// find
    ///
    /// finds the value within the unsorted range of [first, last). 
    ///
    /// Returns: The first iterator i in the range [first, last) for which 
    /// the following corresponding conditions hold: *i == value. 
    /// Returns last if no such iterator is found.
    ///
    /// Complexity: At most 'last - first' applications of the corresponding predicate.
    /// This is a linear search and not a binary one. 
    ///
    /// Note: The predicate version of find is find_if and not another variation of find.
    /// This is because both versions would have three parameters and there could be ambiguity.
    ///
    template <typename InputIterator, typename T>
    inline InputIterator
    find(InputIterator first, InputIterator last, const T& value)
    {
        while((first != last) && !(*first == value)) // Note that we always express value comparisons in terms of < or ==.
            ++first;
        return first;
    }



    /// find_if
    ///
    /// finds the value within the unsorted range of [first, last). 
    ///
    /// Returns: The first iterator i in the range [first, last) for which 
    /// the following corresponding conditions hold: pred(*i) != false. 
    /// Returns last if no such iterator is found.
    /// If the sequence of elements to search for (i.e. first2 - last2) is empty,
    /// the find always fails and last1 will be returned.
    ///
    /// Complexity: At most 'last - first' applications of the corresponding predicate.
    ///
    /// Note: The non-predicate version of find_if is find and not another variation of find_if.
    /// This is because both versions would have three parameters and there could be ambiguity.
    ///
    template <typename InputIterator, typename Predicate>
    inline InputIterator
    find_if(InputIterator first, InputIterator last, Predicate predicate)
    {
        while((first != last) && !predicate(*first))
            ++first;
        return first;
    }



    /// find_first_of
    ///
    /// find_first_of is similar to find in that it performs linear search through 
    /// a range of ForwardIterators. The difference is that while find searches 
    /// for one particular value, find_first_of searches for any of several values. 
    /// Specifically, find_first_of searches for the first occurrance in the 
    /// range [first1, last1) of any of the elements in [first2, last2). 
    /// This function is thus similar to the strpbrk standard C string function.
    /// If the sequence of elements to search for (i.e. first2-last2) is empty,
    /// the find always fails and last1 will be returned.
    ///
    /// Effects: Finds an element that matches one of a set of values.
    ///
    /// Returns: The first iterator i in the range [first1, last1) such that for some 
    /// integer j in the range [first2, last2) the following conditions hold: *i == *j.
    /// Returns last1 if no such iterator is found.
    ///
    /// Complexity: At most '(last1 - first1) * (last2 - first2)' applications of the 
    /// corresponding predicate.
    ///
    template <typename ForwardIterator1, typename ForwardIterator2>
    ForwardIterator1
    find_first_of(ForwardIterator1 first1, ForwardIterator1 last1, 
                  ForwardIterator2 first2, ForwardIterator2 last2)
    {
        for(; first1 != last1; ++first1)
        {
            for(ForwardIterator2 i = first2; i != last2; ++i)
            {
                if(*first1 == *i)
                    return first1;
            }
        }
        return last1;
    }


    /// find_first_of
    ///
    /// find_first_of is similar to find in that it performs linear search through 
    /// a range of ForwardIterators. The difference is that while find searches 
    /// for one particular value, find_first_of searches for any of several values. 
    /// Specifically, find_first_of searches for the first occurrance in the 
    /// range [first1, last1) of any of the elements in [first2, last2). 
    /// This function is thus similar to the strpbrk standard C string function.
    ///
    /// Effects: Finds an element that matches one of a set of values.
    ///
    /// Returns: The first iterator i in the range [first1, last1) such that for some 
    /// integer j in the range [first2, last2) the following conditions hold: pred(*i, *j) != false.
    /// Returns last1 if no such iterator is found.
    ///
    /// Complexity: At most '(last1 - first1) * (last2 - first2)' applications of the 
    /// corresponding predicate.
    ///
    template <typename ForwardIterator1, typename ForwardIterator2, typename BinaryPredicate>
    ForwardIterator1
    find_first_of(ForwardIterator1 first1, ForwardIterator1 last1, 
                  ForwardIterator2 first2, ForwardIterator2 last2,
                  BinaryPredicate predicate)
    {
        for(; first1 != last1; ++first1)
        {
            for(ForwardIterator2 i = first2; i != last2; ++i)
            {
                if(predicate(*first1, *i))
                    return first1;
            }
        }
        return last1;
    }


    /// find_first_not_of
    ///
    /// Searches through first range for the first element that does not belong the second input range.
    /// This is very much like the C++ string find_first_not_of function.
    ///
    /// Returns: The first iterator i in the range [first1, last1) such that for some 
    /// integer j in the range [first2, last2) the following conditions hold: !(*i == *j).
    /// Returns last1 if no such iterator is found.
    ///
    /// Complexity: At most '(last1 - first1) * (last2 - first2)' applications of the 
    /// corresponding predicate.
    ///
    template<class ForwardIterator1, class ForwardIterator2>
    ForwardIterator1
    find_first_not_of(ForwardIterator1 first1, ForwardIterator1 last1, 
                      ForwardIterator2 first2, ForwardIterator2 last2)
    {
        for(; first1 != last1; ++first1)
        {
            if(eastl::find(first2, last2, *first1) == last2)
                break;
        }

        return first1;
    }



    /// find_first_not_of
    ///
    /// Searches through first range for the first element that does not belong the second input range.
    /// This is very much like the C++ string find_first_not_of function.
    ///
    /// Returns: The first iterator i in the range [first1, last1) such that for some 
    /// integer j in the range [first2, last2) the following conditions hold: pred(*i, *j) == false.
    /// Returns last1 if no such iterator is found.
    ///
    /// Complexity: At most '(last1 - first1) * (last2 - first2)' applications of the 
    /// corresponding predicate.
    ///
    template<class ForwardIterator1, class ForwardIterator2, class BinaryPredicate>
    inline ForwardIterator1
    find_first_not_of(ForwardIterator1 first1, ForwardIterator1 last1, 
                      ForwardIterator2 first2, ForwardIterator2 last2, 
                      BinaryPredicate predicate)
    {
        typedef typename eastl::iterator_traits<ForwardIterator1>::value_type value_type;

        for(; first1 != last1; ++first1)
        {
            if(eastl::find_if(first2, last2, eastl::bind1st<BinaryPredicate, value_type>(predicate, *first1)) == last2)
                break;
        }

        return first1;
    }


    template<class BidirectionalIterator1, class ForwardIterator2>
    inline BidirectionalIterator1
    find_last_of(BidirectionalIterator1 first1, BidirectionalIterator1 last1, 
                 ForwardIterator2 first2, ForwardIterator2 last2)
    {
        if((first1 != last1) && (first2 != last2))
        {
            BidirectionalIterator1 it1(last1);

            while((--it1 != first1) && (eastl::find(first2, last2, *it1) == last2))
                ; // Do nothing

            if((it1 != first1) || (eastl::find(first2, last2, *it1) != last2))
                return it1;
        }

        return last1;
    }


    template<class BidirectionalIterator1, class ForwardIterator2, class BinaryPredicate>
    BidirectionalIterator1
    find_last_of(BidirectionalIterator1 first1, BidirectionalIterator1 last1, 
                 ForwardIterator2 first2, ForwardIterator2 last2, 
                 BinaryPredicate predicate)
    {
        typedef typename eastl::iterator_traits<BidirectionalIterator1>::value_type value_type;

        if((first1 != last1) && (first2 != last2))
        {
            BidirectionalIterator1 it1(last1);

            while((--it1 != first1) && (eastl::find_if(first2, last2, eastl::bind1st<BinaryPredicate, value_type>(predicate, *it1)) == last2))
                ; // Do nothing

            if((it1 != first1) || (eastl::find_if(first2, last2, eastl::bind1st<BinaryPredicate, value_type>(predicate, *it1)) != last2))
                return it1;
        }

        return last1;
    }


    template<class BidirectionalIterator1, class ForwardIterator2>
    inline BidirectionalIterator1
    find_last_not_of(BidirectionalIterator1 first1, BidirectionalIterator1 last1, 
                     ForwardIterator2 first2, ForwardIterator2 last2)
    {
        if((first1 != last1) && (first2 != last2))
        {
            BidirectionalIterator1 it1(last1);

            while((--it1 != first1) && (eastl::find(first2, last2, *it1) != last2))
                ; // Do nothing

            if((it1 != first1) || (eastl::find( first2, last2, *it1) == last2))
                return it1;
        }

        return last1;
    }


    template<class BidirectionalIterator1, class ForwardIterator2, class BinaryPredicate>
    inline BidirectionalIterator1
    find_last_not_of(BidirectionalIterator1 first1, BidirectionalIterator1 last1, 
                     ForwardIterator2 first2, ForwardIterator2 last2, 
                     BinaryPredicate predicate)
    {
        typedef typename eastl::iterator_traits<BidirectionalIterator1>::value_type value_type;

        if((first1 != last1) && (first2 != last2))
        {
            BidirectionalIterator1 it1(last1);

            while((--it1 != first1) && (eastl::find_if(first2, last2, eastl::bind1st<BinaryPredicate, value_type>(predicate, *it1)) != last2))
                ; // Do nothing

            if((it1 != first1) || (eastl::find_if(first2, last2, eastl::bind1st<BinaryPredicate, value_type>(predicate, *it1))) == last2)
                return it1;
        }

        return last1;
    }




    /// for_each
    ///
    /// Calls the Function function for each value in the range [first, last).
    /// Function takes a single parameter: the current value.
    /// 
    /// Effects: Applies function to the result of dereferencing every iterator in 
    /// the range [first, last), starting from first and proceeding to last 1.
    ///
    /// Returns: function.
    ///
    /// Complexity: Applies function exactly 'last - first' times.
    ///
    /// Note: If function returns a result, the result is ignored.
    /// 
    template <typename InputIterator, typename Function>
    inline Function
    for_each(InputIterator first, InputIterator last, Function function)
    {
        for(; first != last; ++first)
            function(*first);
        return function;
    }


    /// generate
    ///
    /// Iterates the range of [first, last) and assigns to each element the
    /// result of the function generator. Generator is a function which takes
    /// no arguments.
    /// 
    /// Complexity: Exactly 'last - first' invocations of generator and assignments.
    ///
    template <typename ForwardIterator, typename Generator>
    inline void
    generate(ForwardIterator first, ForwardIterator last, Generator generator)
    {
        for(; first != last; ++first) // We cannot call generate_n(first, last-first, generator) 
            *first = generator();     // because the 'last-first' might not be supported by the 
    }                                 // given iterator.


    /// generate_n
    ///
    /// Iterates an interator n times and assigns the result of generator
    /// to each succeeding element. Generator is a function which takes
    /// no arguments.
    /// 
    /// Complexity: Exactly n invocations of generator and assignments.
    ///
    template <typename OutputIterator, typename Size, typename Generator>
    inline OutputIterator
    generate_n(OutputIterator first, Size n, Generator generator)
    {
        for(; n > 0; --n, ++first)
            *first = generator();
        return first;
    }


    /// transform
    ///
    /// Iterates the input range of [first, last) and the output iterator result
    /// and assigns the result of unaryOperation(input) to result.
    /// 
    /// Effects: Assigns through every iterator i in the range [result, result + (last1 - first1))
    /// a new corresponding value equal to unaryOperation(*(first1 + (i - result)).
    ///
    /// Requires: op shall not have any side effects.
    ///
    /// Returns: result + (last1 - first1). That is, returns the end of the output range.
    ///
    /// Complexity: Exactly 'last1 - first1' applications of unaryOperation.
    ///
    /// Note: result may be equal to first.
    ///
    template <typename InputIterator, typename OutputIterator, typename UnaryOperation>
    inline OutputIterator
    transform(InputIterator first, InputIterator last, OutputIterator result, UnaryOperation unaryOperation)
    {
        for(; first != last; ++first, ++result)
            *result = unaryOperation(*first);
        return result;
    }


    /// transform
    ///
    /// Iterates the input range of [first, last) and the output iterator result
    /// and assigns the result of binaryOperation(input1, input2) to result.
    /// 
    /// Effects: Assigns through every iterator i in the range [result, result + (last1 - first1))
    /// a new corresponding value equal to binaryOperation(*(first1 + (i - result), *(first2 + (i - result))).
    ///
    /// Requires: binaryOperation shall not have any side effects.
    ///
    /// Returns: result + (last1 - first1). That is, returns the end of the output range.
    ///
    /// Complexity: Exactly 'last1 - first1' applications of binaryOperation.
    ///
    /// Note: result may be equal to first1 or first2.
    ///
    template <typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryOperation>
    inline OutputIterator
    transform(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, OutputIterator result, BinaryOperation binaryOperation)
    {
        for(; first1 != last1; ++first1, ++first2, ++result)
            *result = binaryOperation(*first1, *first2);
        return result;
    }


    /// equal
    ///
    /// Returns: true if for every iterator i in the range [first1, last1) the 
    /// following corresponding conditions hold: predicate(*i, *(first2 + (i - first1))) != false. 
    /// Otherwise, returns false.
    ///
    /// Complexity: At most last1 first1 applications of the corresponding predicate.
    ///
    /// To consider: Make specializations of this for scalar types and random access
    /// iterators that uses memcmp or some trick memory comparison function. 
    /// We should verify that such a thing results in an improvement.
    ///
    template <typename InputIterator1, typename InputIterator2>
    inline bool
    equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2)
    {
        for(; first1 != last1; ++first1, ++first2)
        {
            if(!(*first1 == *first2)) // Note that we always express value comparisons in terms of < or ==.
                return false;
        }
        return true;
    }

    /// equal
    ///
    /// Returns: true if for every iterator i in the range [first1, last1) the 
    /// following corresponding conditions hold: pred(*i, *(first2 + (i first1))) != false. 
    /// Otherwise, returns false.
    ///
    /// Complexity: At most last1 first1 applications of the corresponding predicate.
    ///
    template <typename InputIterator1, typename InputIterator2, typename BinaryPredicate>
    inline bool
    equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, BinaryPredicate predicate)
    {
        for(; first1 != last1; ++first1, ++first2)
        {
            if(!predicate(*first1, *first2))
                return false;
        }
        return true;
    }



    /// identical
    ///
    /// Returns true if the two input ranges are equivalent.
    /// There is a subtle difference between this algorithm and 
    /// the 'equal' algorithm. The equal algorithm assumes the 
    /// two ranges are of equal length. This algorithm efficiently
    /// compares two ranges for both length equality and for 
    /// element equality. There is no other standard algorithm
    /// that can do this.
    ///
    /// Returns: true if the sequence of elements defined by the range 
    /// [first1, last1) is of the same length as the sequence of
    /// elements defined by the range of [first2, last2) and if
    /// the elements in these ranges are equal as per the 
    /// equal algorithm.
    ///
    /// Complexity: At most 'min((last1 - first1), (last2 - first2))' applications 
    /// of the corresponding comparison.
    ///
    template <typename InputIterator1, typename InputIterator2>
    bool identical(InputIterator1 first1, InputIterator1 last1,
                   InputIterator2 first2, InputIterator2 last2)
    {
        while((first1 != last1) && (first2 != last2) && (*first1 == *first2))
        {
            ++first1;
            ++first2;
        }
        return (first1 == last1) && (first2 == last2);
    }


    /// identical
    ///
    template <typename InputIterator1, typename InputIterator2, typename BinaryPredicate>
    bool identical(InputIterator1 first1, InputIterator1 last1,
                   InputIterator2 first2, InputIterator2 last2, BinaryPredicate predicate)
    {
        while((first1 != last1) && (first2 != last2) && predicate(*first1, *first2))
        {
            ++first1;
            ++first2;
        }
        return (first1 == last1) && (first2 == last2);
    }


    /// lexicographical_compare
    ///
    /// Returns: true if the sequence of elements defined by the range 
    /// [first1, last1) is lexicographically less than the sequence of 
    /// elements defined by the range [first2, last2). Returns false otherwise.
    ///
    /// Complexity: At most 'min((last1 - first1), (last2 - first2))' applications 
    /// of the corresponding comparison.
    ///
    /// Note: If two sequences have the same number of elements and their 
    /// corresponding elements are equivalent, then neither sequence is 
    /// lexicographically less than the other. If one sequence is a prefix 
    /// of the other, then the shorter sequence is lexicographically less 
    /// than the longer sequence. Otherwise, the lexicographical comparison 
    /// of the sequences yields the same result as the comparison of the first 
    /// corresponding pair of elements that are not equivalent.
    ///
    template <typename InputIterator1, typename InputIterator2>
    inline bool
    lexicographical_compare(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2)
    {
        for(; (first1 != last1) && (first2 != last2); ++first1, ++first2)
        {
            if(*first1 < *first2)
                return true;
            if(*first2 < *first1)
                return false;
        }
        return (first1 == last1) && (first2 != last2);
    }

    inline bool     // Specialization for const char*.
    lexicographical_compare(const char* first1, const char* last1, const char* first2, const char* last2)
    {
        const ptrdiff_t n1(last1 - first1), n2(last2 - first2);
        const int result = memcmp(first1, first2, (size_t)eastl::min_alt(n1, n2));
        return result ? (result < 0) : (n1 < n2);
    }

    inline bool     // Specialization for char*.
    lexicographical_compare(char* first1, char* last1, char* first2, char* last2)
    {
        const ptrdiff_t n1(last1 - first1), n2(last2 - first2);
        const int result = memcmp(first1, first2, (size_t)eastl::min_alt(n1, n2));
        return result ? (result < 0) : (n1 < n2);
    }

    inline bool     // Specialization for const unsigned char*.
    lexicographical_compare(const unsigned char* first1, const unsigned char* last1, const unsigned char* first2, const unsigned char* last2)
    {
        const ptrdiff_t n1(last1 - first1), n2(last2 - first2);
        const int result = memcmp(first1, first2, (size_t)eastl::min_alt(n1, n2));
        return result ? (result < 0) : (n1 < n2);
    }

    inline bool     // Specialization for unsigned char*.
    lexicographical_compare(unsigned char* first1, unsigned char* last1, unsigned char* first2, unsigned char* last2)
    {
        const ptrdiff_t n1(last1 - first1), n2(last2 - first2);
        const int result = memcmp(first1, first2, (size_t)eastl::min_alt(n1, n2));
        return result ? (result < 0) : (n1 < n2);
    }

    inline bool     // Specialization for const signed char*.
    lexicographical_compare(const signed char* first1, const signed char* last1, const signed char* first2, const signed char* last2)
    {
        const ptrdiff_t n1(last1 - first1), n2(last2 - first2);
        const int result = memcmp(first1, first2, (size_t)eastl::min_alt(n1, n2));
        return result ? (result < 0) : (n1 < n2);
    }

    inline bool     // Specialization for signed char*.
    lexicographical_compare(signed char* first1, signed char* last1, signed char* first2, signed char* last2)
    {
        const ptrdiff_t n1(last1 - first1), n2(last2 - first2);
        const int result = memcmp(first1, first2, (size_t)eastl::min_alt(n1, n2));
        return result ? (result < 0) : (n1 < n2);
    }

    #if defined(_MSC_VER) // If using the VC++ compiler (and thus bool is known to be a single byte)...
        //Not sure if this is a good idea.
        //inline bool     // Specialization for const bool*.
        //lexicographical_compare(const bool* first1, const bool* last1, const bool* first2, const bool* last2)
        //{
        //    const ptrdiff_t n1(last1 - first1), n2(last2 - first2);
        //    const int result = memcmp(first1, first2, (size_t)eastl::min_alt(n1, n2));
        //    return result ? (result < 0) : (n1 < n2);
        //}
        //
        //inline bool     // Specialization for bool*.
        //lexicographical_compare(bool* first1, bool* last1, bool* first2, bool* last2)
        //{
        //    const ptrdiff_t n1(last1 - first1), n2(last2 - first2);
        //    const int result = memcmp(first1, first2, (size_t)eastl::min_alt(n1, n2));
        //    return result ? (result < 0) : (n1 < n2);
        //}
    #endif



    /// lexicographical_compare
    ///
    /// Returns: true if the sequence of elements defined by the range 
    /// [first1, last1) is lexicographically less than the sequence of 
    /// elements defined by the range [first2, last2). Returns false otherwise.
    ///
    /// Complexity: At most 'min((last1 -first1), (last2 - first2))' applications 
    /// of the corresponding comparison.
    ///
    /// Note: If two sequences have the same number of elements and their 
    /// corresponding elements are equivalent, then neither sequence is 
    /// lexicographically less than the other. If one sequence is a prefix 
    /// of the other, then the shorter sequence is lexicographically less 
    /// than the longer sequence. Otherwise, the lexicographical comparison 
    /// of the sequences yields the same result as the comparison of the first 
    /// corresponding pair of elements that are not equivalent.
    ///
    /// Note: False is always returned if range 1 is exhausted before range 2.
    /// The result of this is that you can't do a successful reverse compare
    /// (e.g. use greater<> as the comparison instead of less<>) unless the 
    /// two sequences are of identical length. What you want to do is reverse
    /// the order of the arguments in order to get the desired effect.
    ///
    template <typename InputIterator1, typename InputIterator2, typename Compare>
    inline bool
    lexicographical_compare(InputIterator1 first1, InputIterator1 last1, 
                            InputIterator2 first2, InputIterator2 last2, Compare compare)
    {
        for(; (first1 != last1) && (first2 != last2); ++first1, ++first2)
        {
            if(compare(*first1, *first2))
                return true;
            if(compare(*first2, *first1))
                return false;
        }
        return (first1 == last1) && (first2 != last2);
    }



    /// lower_bound
    ///
    /// Finds the position of the first element in a sorted range that has a value 
    /// greater than or equivalent to a specified value.
    ///
    /// Effects: Finds the first position into which value can be inserted without 
    /// violating the ordering.
    /// 
    /// Returns: The furthermost iterator i in the range [first, last) such that 
    /// for any iterator j in the range [first, i) the following corresponding 
    /// condition holds: *j < value.
    ///
    /// Complexity: At most 'log(last - first) + 1' comparisons.
    ///
    /// Optimizations: We have no need to specialize this implementation for random 
    /// access iterators (e.g. contiguous array), as the code below will already 
    /// take advantage of them.
    ///
    template <typename ForwardIterator, typename T>
    ForwardIterator
    lower_bound(ForwardIterator first, ForwardIterator last, const T& value)
    {
        typedef typename eastl::iterator_traits<ForwardIterator>::difference_type DifferenceType;

        DifferenceType d = eastl::distance(first, last); // This will be efficient for a random access iterator such as an array.

        while(d > 0)
        {
            ForwardIterator i  = first;
            DifferenceType  d2 = d >> 1; // We use '>>1' here instead of '/2' because MSVC++ for some reason generates significantly worse code for '/2'. Go figure.

            eastl::advance(i, d2); // This will be efficient for a random access iterator such as an array.

            if(*i < value)
            {
                // Disabled because std::lower_bound doesn't specify (23.3.3.3, p3) this can be done: EASTL_VALIDATE_COMPARE(!(value < *i)); // Validate that the compare function is sane.
                first = ++i;
                d    -= d2 + 1;
            }
            else
                d = d2;
        }
        return first;
    }


    /// lower_bound
    ///
    /// Finds the position of the first element in a sorted range that has a value 
    /// greater than or equivalent to a specified value. The input Compare function
    /// takes two arguments and returns true if the first argument is less than
    /// the second argument.
    ///
    /// Effects: Finds the first position into which value can be inserted without 
    /// violating the ordering.
    /// 
    /// Returns: The furthermost iterator i in the range [first, last) such that 
    /// for any iterator j in the range [first, i) the following corresponding 
    /// condition holds: compare(*j, value) != false.
    ///
    /// Complexity: At most 'log(last - first) + 1' comparisons.
    ///
    /// Optimizations: We have no need to specialize this implementation for random 
    /// access iterators (e.g. contiguous array), as the code below will already 
    /// take advantage of them.
    ///
    template <typename ForwardIterator, typename T, typename Compare>
    ForwardIterator
    lower_bound(ForwardIterator first, ForwardIterator last, const T& value, Compare compare)
    {
        typedef typename eastl::iterator_traits<ForwardIterator>::difference_type DifferenceType;

        DifferenceType d = eastl::distance(first, last); // This will be efficient for a random access iterator such as an array.

        while(d > 0)
        {
            ForwardIterator i  = first;
            DifferenceType  d2 = d >> 1; // We use '>>1' here instead of '/2' because MSVC++ for some reason generates significantly worse code for '/2'. Go figure.

            eastl::advance(i, d2); // This will be efficient for a random access iterator such as an array.

            if(compare(*i, value))
            {
                // Disabled because std::lower_bound doesn't specify (23.3.3.1, p3) this can be done: EASTL_VALIDATE_COMPARE(!compare(value, *i)); // Validate that the compare function is sane.
                first = ++i;
                d    -= d2 + 1;
            }
            else
                d = d2;
        }
        return first;
    }



    /// upper_bound
    ///
    /// Finds the position of the first element in a sorted range that has a 
    /// value that is greater than a specified value.
    ///
    /// Effects: Finds the furthermost position into which value can be inserted 
    /// without violating the ordering.
    ///
    /// Returns: The furthermost iterator i in the range [first, last) such that 
    /// for any iterator j in the range [first, i) the following corresponding 
    /// condition holds: !(value < *j).
    ///
    /// Complexity: At most 'log(last - first) + 1' comparisons.
    ///
    template <typename ForwardIterator, typename T>
    ForwardIterator
    upper_bound(ForwardIterator first, ForwardIterator last, const T& value)
    {
        typedef typename eastl::iterator_traits<ForwardIterator>::difference_type DifferenceType;

        DifferenceType len = eastl::distance(first, last);

        while(len > 0)
        {
            ForwardIterator i    = first;
            DifferenceType  len2 = len >> 1; // We use '>>1' here instead of '/2' because MSVC++ for some reason generates significantly worse code for '/2'. Go figure.

            eastl::advance(i, len2);

            if(!(value < *i)) // Note that we always express value comparisons in terms of < or ==.
            {
                first = ++i;
                len -= len2 + 1;
            }
            else
            {
                // Disabled because std::upper_bound doesn't specify (23.3.3.2, p3) this can be done: EASTL_VALIDATE_COMPARE(!(*i < value)); // Validate that the compare function is sane.
                len = len2;
            }
        }
        return first;
    }


    /// upper_bound
    ///
    /// Finds the position of the first element in a sorted range that has a 
    /// value that is greater than a specified value. The input Compare function
    /// takes two arguments and returns true if the first argument is less than
    /// the second argument.
    ///
    /// Effects: Finds the furthermost position into which value can be inserted 
    /// without violating the ordering.
    ///
    /// Returns: The furthermost iterator i in the range [first, last) such that 
    /// for any iterator j in the range [first, i) the following corresponding 
    /// condition holds: compare(value, *j) == false.
    ///
    /// Complexity: At most 'log(last - first) + 1' comparisons.
    ///
    template <typename ForwardIterator, typename T, typename Compare>
    ForwardIterator
    upper_bound(ForwardIterator first, ForwardIterator last, const T& value, Compare compare)
    {
        typedef typename eastl::iterator_traits<ForwardIterator>::difference_type DifferenceType;

        DifferenceType len = eastl::distance(first, last);

        while(len > 0)
        {
            ForwardIterator i    = first;
            DifferenceType  len2 = len >> 1; // We use '>>1' here instead of '/2' because MSVC++ for some reason generates significantly worse code for '/2'. Go figure.

            eastl::advance(i, len2);

            if(!compare(value, *i))
            {
                first = ++i;
                len -= len2 + 1;
            }
            else
            {
                // Disabled because std::upper_bound doesn't specify (23.3.3.2, p3) this can be done: EASTL_VALIDATE_COMPARE(!compare(*i, value)); // Validate that the compare function is sane.
                len = len2;
            }
        }
        return first;
    }


    /// equal_range
    ///
    /// Effects: Finds the largest subrange [i, j) such that the value can be inserted 
    /// at any iterator k in it without violating the ordering. k satisfies the 
    /// corresponding conditions: !(*k < value) && !(value < *k).
    /// 
    /// Complexity: At most '2 * log(last - first) + 1' comparisons.
    ///
    template <typename ForwardIterator, typename T>
    pair<ForwardIterator, ForwardIterator>
    equal_range(ForwardIterator first, ForwardIterator last, const T& value)
    {
        typedef pair<ForwardIterator, ForwardIterator> ResultType;
        typedef typename eastl::iterator_traits<ForwardIterator>::difference_type DifferenceType;

        DifferenceType d = eastl::distance(first, last);

        while(d > 0)
        {
            ForwardIterator i(first);
            DifferenceType  d2 = d >> 1; // We use '>>1' here instead of '/2' because MSVC++ for some reason generates significantly worse code for '/2'. Go figure.

            eastl::advance(i, d2);

            if(*i < value)
            {
                EASTL_VALIDATE_COMPARE(!(value < *i)); // Validate that the compare function is sane.
                first = ++i;
                d    -= d2 + 1;
            }
            else if(value < *i)
            {
                EASTL_VALIDATE_COMPARE(!(*i < value)); // Validate that the compare function is sane.
                d    = d2;
                last = i;
            }
            else
            {
                ForwardIterator j(i);

                return ResultType(eastl::lower_bound(first, i, value), 
                                  eastl::upper_bound(++j, last, value));
            }
        }
        return ResultType(first, first);
    }


    /// equal_range
    ///
    /// Effects: Finds the largest subrange [i, j) such that the value can be inserted 
    /// at any iterator k in it without violating the ordering. k satisfies the 
    /// corresponding conditions: comp(*k, value) == false && comp(value, *k) == false.
    /// 
    /// Complexity: At most '2 * log(last - first) + 1' comparisons.
    ///
    template <typename ForwardIterator, typename T, typename Compare>
    pair<ForwardIterator, ForwardIterator>
    equal_range(ForwardIterator first, ForwardIterator last, const T& value, Compare compare)
    {
        typedef pair<ForwardIterator, ForwardIterator> ResultType;
        typedef typename eastl::iterator_traits<ForwardIterator>::difference_type DifferenceType;

        DifferenceType d = eastl::distance(first, last);

        while(d > 0)
        {
            ForwardIterator i(first);
            DifferenceType  d2 = d >> 1; // We use '>>1' here instead of '/2' because MSVC++ for some reason generates significantly worse code for '/2'. Go figure.

            eastl::advance(i, d2);

            if(compare(*i, value))
            {
                EASTL_VALIDATE_COMPARE(!compare(value, *i)); // Validate that the compare function is sane.
                first = ++i;
                d    -= d2 + 1;
            }
            else if(compare(value, *i))
            {
                EASTL_VALIDATE_COMPARE(!compare(*i, value)); // Validate that the compare function is sane.
                d    = d2;
                last = i;
            }
            else
            {
                ForwardIterator j(i);

                return ResultType(eastl::lower_bound(first, i, value, compare), 
                                  eastl::upper_bound(++j, last, value, compare));
            }
        }
        return ResultType(first, first);
    }


    /// replace
    ///
    /// Effects: Substitutes elements referred by the iterator i in the range [first, last) 
    /// with new_value, when the following corresponding conditions hold: *i == old_value.
    /// 
    /// Complexity: Exactly 'last - first' applications of the corresponding predicate.
    ///
    /// Note: The predicate version of replace is replace_if and not another variation of replace.
    /// This is because both versions would have the same parameter count and there could be ambiguity.
    ///
    template <typename ForwardIterator, typename T>
    inline void
    replace(ForwardIterator first, ForwardIterator last, const T& old_value, const T& new_value)
    {
        for(; first != last; ++first)
        {
            if(*first == old_value)
                *first = new_value;
        }
    }


    /// replace_if
    ///
    /// Effects: Substitutes elements referred by the iterator i in the range [first, last) 
    /// with new_value, when the following corresponding conditions hold: predicate(*i) != false.
    /// 
    /// Complexity: Exactly 'last - first' applications of the corresponding predicate.
    ///
    /// Note: The predicate version of replace_if is replace and not another variation of replace_if.
    /// This is because both versions would have the same parameter count and there could be ambiguity.
    ///
    template <typename ForwardIterator, typename Predicate, typename T>
    inline void
    replace_if(ForwardIterator first, ForwardIterator last, Predicate predicate, const T& new_value)
    {
        for(; first != last; ++first)
        {
            if(predicate(*first))
                *first = new_value;
        }
    }


    /// remove_copy
    ///
    /// Effects: Copies all the elements referred to by the iterator i in the range 
    /// [first, last) for which the following corresponding condition does not hold: 
    /// *i == value.
    ///
    /// Requires: The ranges [first, last) and [result, result + (last - first)) shall not overlap.
    ///
    /// Returns: The end of the resulting range.
    ///
    /// Complexity: Exactly 'last - first' applications of the corresponding predicate.
    ///
    template <typename InputIterator, typename OutputIterator, typename T>
    inline OutputIterator
    remove_copy(InputIterator first, InputIterator last, OutputIterator result, const T& value)
    {
        for(; first != last; ++first)
        {
            if(!(*first == value)) // Note that we always express value comparisons in terms of < or ==.
            {
                *result = *first;
                ++result;
            }
        }
        return result;
    }


    /// remove_copy_if
    ///
    /// Effects: Copies all the elements referred to by the iterator i in the range 
    /// [first, last) for which the following corresponding condition does not hold: 
    /// predicate(*i) != false.
    ///
    /// Requires: The ranges [first, last) and [result, result + (last - first)) shall not overlap.
    ///
    /// Returns: The end of the resulting range.
    ///
    /// Complexity: Exactly 'last - first' applications of the corresponding predicate.
    ///
    template <typename InputIterator, typename OutputIterator, typename Predicate>
    inline OutputIterator
    remove_copy_if(InputIterator first, InputIterator last, OutputIterator result, Predicate predicate)
    {
        for(; first != last; ++first)
        {
            if(!predicate(*first))
            {
                *result = *first;
                ++result;
            }
        }
        return result;
    }


    /// remove
    ///
    /// Effects: Eliminates all the elements referred to by iterator i in the 
    /// range [first, last) for which the following corresponding condition
    /// holds: *i == value.
    ///
    /// Returns: The end of the resulting range.
    ///
    /// Complexity: Exactly 'last - first' applications of the corresponding predicate.
    ///
    /// Note: The predicate version of remove is remove_if and not another variation of remove.
    /// This is because both versions would have the same parameter count and there could be ambiguity.
    ///
    /// Note: Since this function moves the element to the back of the heap and 
    /// doesn't actually remove it from the given container, the user must call
    /// the container erase function if the user wants to erase the element 
    /// from the container.
    ///
    /// Example usage:
    ///    vector<int> intArray;
    ///    ...
    ///    intArray.erase(remove(intArray.begin(), intArray.end(), 4), intArray.end()); // Erase all elements of value 4.
    ///
    template <typename ForwardIterator, typename T>
    inline ForwardIterator
    remove(ForwardIterator first, ForwardIterator last, const T& value)
    {
        first = eastl::find(first, last, value);
        if(first != last)
        {
            ForwardIterator i(first);
            return eastl::remove_copy(++i, last, first, value);
        }
        return first;
    }


    /// remove_if
    ///
    /// Effects: Eliminates all the elements referred to by iterator i in the 
    /// range [first, last) for which the following corresponding condition 
    /// holds: predicate(*i) != false.
    ///
    /// Returns: The end of the resulting range.
    ///
    /// Complexity: Exactly 'last - first' applications of the corresponding predicate.
    ///
    /// Note: The predicate version of remove_if is remove and not another variation of remove_if.
    /// This is because both versions would have the same parameter count and there could be ambiguity.
    ///
    /// Note: Since this function moves the element to the back of the heap and 
    /// doesn't actually remove it from the given container, the user must call
    /// the container erase function if the user wants to erase the element 
    /// from the container.
    ///
    /// Example usage:
    ///    vector<int> intArray;
    ///    ...
    ///    intArray.erase(remove(intArray.begin(), intArray.end(), bind2nd(less<int>(), (int)3)), intArray.end()); // Erase all elements less than 3.
    ///
    template <typename ForwardIterator, typename Predicate>
    inline ForwardIterator
    remove_if(ForwardIterator first, ForwardIterator last, Predicate predicate)
    {
        first = eastl::find_if(first, last, predicate);
        if(first != last)
        {
            ForwardIterator i(first);
            return eastl::remove_copy_if<ForwardIterator, ForwardIterator, Predicate>(++i, last, first, predicate);
        }
        return first;
    }


    /// replace_copy
    ///
    /// Effects: Assigns to every iterator i in the range [result, result + (last - first))
    /// either new_value or *(first + (i - result)) depending on whether the following 
    /// corresponding conditions hold: *(first + (i - result)) == old_value.
    ///
    /// Requires: The ranges [first, last) and [result, result + (last - first)) shall not overlap.
    ///
    /// Returns: result + (last - first).
    ///
    /// Complexity: Exactly 'last - first' applications of the corresponding predicate.
    ///
    /// Note: The predicate version of replace_copy is replace_copy_if and not another variation of replace_copy.
    /// This is because both versions would have the same parameter count and there could be ambiguity.
    ///
    template <typename InputIterator, typename OutputIterator, typename T>
    inline OutputIterator
    replace_copy(InputIterator first, InputIterator last, OutputIterator result, const T& old_value, const T& new_value)
    {
        for(; first != last; ++first, ++result)
            *result = (*first == old_value) ? new_value : *first;
        return result;
    }


    /// replace_copy_if
    ///
    /// Effects: Assigns to every iterator i in the range [result, result + (last - first))
    /// either new_value or *(first + (i - result)) depending on whether the following 
    /// corresponding conditions hold: predicate(*(first + (i - result))) != false.
    ///
    /// Requires: The ranges [first, last) and [result, result+(lastfirst)) shall not overlap.
    ///
    /// Returns: result + (last - first).
    ///
    /// Complexity: Exactly 'last - first' applications of the corresponding predicate.
    ///
    /// Note: The predicate version of replace_copy_if is replace_copy and not another variation of replace_copy_if.
    /// This is because both versions would have the same parameter count and there could be ambiguity.
    ///
    template <typename InputIterator, typename OutputIterator, typename Predicate, typename T>
    inline OutputIterator
    replace_copy_if(InputIterator first, InputIterator last, OutputIterator result, Predicate predicate, const T& new_value)
    {
        for(; first != last; ++first, ++result)
            *result = predicate(*first) ? new_value : *first;
        return result;
    }




    // reverse
    //
    // We provide helper functions which allow reverse to be implemented more  
    // efficiently for some types of iterators and types.
    //
    template <typename BidirectionalIterator>
    inline void reverse_impl(BidirectionalIterator first, BidirectionalIterator last, EASTL_ITC_NS::bidirectional_iterator_tag)
    {
        for(; (first != last) && (first != --last); ++first) // We are not allowed to use operator <, <=, >, >= with a
            eastl::iter_swap(first, last);                   // generic (bidirectional or otherwise) iterator.
    }

    template <typename RandomAccessIterator>
    inline void reverse_impl(RandomAccessIterator first, RandomAccessIterator last, EASTL_ITC_NS::random_access_iterator_tag)
    {
        for(; first < --last; ++first)      // With a random access iterator, we can use operator < to more efficiently implement
            eastl::iter_swap(first, last);  // this algorithm. A generic iterator doesn't necessarily have an operator < defined.
    }

    /// reverse
    ///
    /// Reverses the values within the range [first, last).
    ///
    /// Effects: For each nonnegative integer i <= (last - first) / 2, 
    /// applies swap to all pairs of iterators first + i, (last i) - 1.
    ///
    /// Complexity: Exactly '(last - first) / 2' swaps.
    ///
    template <typename BidirectionalIterator>
    inline void reverse(BidirectionalIterator first, BidirectionalIterator last)
    {
        typedef typename eastl::iterator_traits<BidirectionalIterator>::iterator_category IC;
        eastl::reverse_impl(first, last, IC());
    }



    /// reverse_copy
    ///
    /// Copies the range [first, last) in reverse order to the result.
    ///
    /// Effects: Copies the range [first, last) to the range 
    /// [result, result + (last - first)) such that for any nonnegative
    /// integer i < (last - first) the following assignment takes place:
    /// *(result + (last - first) - i) = *(first + i)
    ///
    /// Requires: The ranges [first, last) and [result, result + (last - first))
    /// shall not overlap.
    ///
    /// Returns: result + (last - first). That is, returns the end of the output range.
    ///
    /// Complexity: Exactly 'last - first' assignments.
    ///
    template <typename BidirectionalIterator, typename OutputIterator>
    inline OutputIterator
    reverse_copy(BidirectionalIterator first, BidirectionalIterator last, OutputIterator result)
    {
        for(; first != last; ++result)
            *result = *--last;
        return result;
    }



    /// search
    ///
    /// Search finds a subsequence within the range [first1, last1) that is identical to [first2, last2) 
    /// when compared element-by-element. It returns an iterator pointing to the beginning of that 
    /// subsequence, or else last1 if no such subsequence exists. As such, it is very much like 
    /// the C strstr function, with the primary difference being that strstr uses 0-terminated strings
    /// whereas search uses an end iterator to specify the end of a string.
    ///
    /// Returns: The first iterator i in the range [first1, last1 - (last2 - first2)) such that for
    /// any nonnegative integer n less than 'last2 - first2' the following corresponding condition holds:
    /// *(i + n) == *(first2 + n). Returns last1 if no such iterator is found.
    ///
    /// Complexity: At most (last1 first1) * (last2 first2) applications of the corresponding predicate.
    ///
    template <typename ForwardIterator1, typename ForwardIterator2>
    ForwardIterator1
    search(ForwardIterator1 first1, ForwardIterator1 last1, 
           ForwardIterator2 first2, ForwardIterator2 last2)
    {
        if(first2 != last2) // If there is anything to search for...
        {
            // We need to make a special case for a pattern of one element,
            // as the logic below prevents one element patterns from working.
            ForwardIterator2 temp2(first2);
            ++temp2;

            if(temp2 != last2) // If what we are searching for has a length > 1...
            {
                ForwardIterator1 cur1(first1);
                ForwardIterator2 p2;

                while(first1 != last1)
                {
                    // The following loop is the equivalent of eastl::find(first1, last1, *first2)
                    while((first1 != last1) && !(*first1 == *first2))
                        ++first1;

                    if(first1 != last1)
                    {
                        p2   = temp2;
                        cur1 = first1;

                        if(++cur1 != last1)
                        {
                            while(*cur1 == *p2)
                            {
                                if(++p2 == last2)
                                    return first1;

                                if(++cur1 == last1)
                                    return last1;
                            }

                            ++first1;
                            continue;
                        }
                    }
                    return last1;
                }

                // Fall through to the end.
            }
            else
                return eastl::find(first1, last1, *first2);
        }

        return first1;


        #if 0
        /*  Another implementation which is a little more simpler but executes a little slower on average.
            typedef typename eastl::iterator_traits<ForwardIterator1>::difference_type difference_type_1;
            typedef typename eastl::iterator_traits<ForwardIterator2>::difference_type difference_type_2;

            const difference_type_2 d2 = eastl::distance(first2, last2);

            for(difference_type_1 d1 = eastl::distance(first1, last1); d1 >= d2; ++first1, --d1)
            {
                ForwardIterator1 temp1 = first1;

                for(ForwardIterator2 temp2 = first2; ; ++temp1, ++temp2)
                {
                    if(temp2 == last2)
                        return first1;
                    if(!(*temp1 == *temp2))
                        break;
                }
            }

            return last1;
        */
        #endif
    }


    /// search
    ///
    /// Search finds a subsequence within the range [first1, last1) that is identical to [first2, last2) 
    /// when compared element-by-element. It returns an iterator pointing to the beginning of that 
    /// subsequence, or else last1 if no such subsequence exists. As such, it is very much like 
    /// the C strstr function, with the only difference being that strstr uses 0-terminated strings
    /// whereas search uses an end iterator to specify the end of a string.
    ///
    /// Returns: The first iterator i in the range [first1, last1 - (last2 - first2)) such that for
    /// any nonnegative integer n less than 'last2 - first2' the following corresponding condition holds:
    /// predicate(*(i + n), *(first2 + n)) != false. Returns last1 if no such iterator is found.
    ///
    /// Complexity: At most (last1 first1) * (last2 first2) applications of the corresponding predicate.
    ///
    template <typename ForwardIterator1, typename ForwardIterator2, typename BinaryPredicate>
    ForwardIterator1
    search(ForwardIterator1 first1, ForwardIterator1 last1, 
           ForwardIterator2 first2, ForwardIterator2 last2,
           BinaryPredicate predicate)
    {
        typedef typename eastl::iterator_traits<ForwardIterator1>::difference_type difference_type_1;
        typedef typename eastl::iterator_traits<ForwardIterator2>::difference_type difference_type_2;

        difference_type_2 d2 = eastl::distance(first2, last2);

        if(d2 != 0)
        {
            ForwardIterator1 i(first1);
            eastl::advance(i, d2);

            for(difference_type_1 d1 = eastl::distance(first1, last1); d1 >= d2; --d1)
            {
                if(eastl::equal<ForwardIterator1, ForwardIterator2, BinaryPredicate>(first1, i, first2, predicate))
                    return first1;
                if(d1 > d2) // To do: Find a way to make the algorithm more elegant.
                {
                    ++first1;
                    ++i;
                }
            }
            return last1;
        }
        return first1; // Just like with strstr, we return first1 if the match string is empty.
    }



    // search_n helper functions
    //
    template <typename ForwardIterator, typename Size, typename T>
    ForwardIterator     // Generic implementation.
    search_n_impl(ForwardIterator first, ForwardIterator last, Size count, const T& value, EASTL_ITC_NS::forward_iterator_tag)
    {
        if(count <= 0)
            return first;

        Size d1 = (Size)eastl::distance(first, last); // Should d1 be of type Size, ptrdiff_t, or iterator_traits<ForwardIterator>::difference_type?
                                                      // The problem with using iterator_traits<ForwardIterator>::difference_type is that 
        if(count > d1)                                // ForwardIterator may not be a true iterator but instead something like a pointer.
            return last;

        for(; d1 >= count; ++first, --d1)
        {
            ForwardIterator i(first);

            for(Size n = 0; n < count; ++n, ++i, --d1)
            {
                if(!(*i == value)) // Note that we always express value comparisons in terms of < or ==.
                    goto not_found;
            }
            return first;

            not_found:
            first = i;
        }
        return last;
    }

    template <typename RandomAccessIterator, typename Size, typename T> inline
    RandomAccessIterator    // Random access iterator implementation. Much faster than generic implementation.
    search_n_impl(RandomAccessIterator first, RandomAccessIterator last, Size count, const T& value, EASTL_ITC_NS::random_access_iterator_tag)
    {
        if(count <= 0)
            return first;
        else if(count == 1)
            return find(first, last, value);
        else if(last > first)
        {
            RandomAccessIterator lookAhead;
            RandomAccessIterator backTrack;

            Size skipOffset = (count - 1);
            Size tailSize = (Size)(last - first);
            Size remainder;
            Size prevRemainder;

            for(lookAhead = first + skipOffset; tailSize >= count; lookAhead += count)
            {
                tailSize -= count;

                if(*lookAhead == value)
                {
                    remainder = skipOffset;

                    for(backTrack = lookAhead - 1; *backTrack == value; --backTrack)
                    {
                        if(--remainder == 0)
                            return (lookAhead - skipOffset); // success
                    }

                    if(remainder <= tailSize)
                    {
                        prevRemainder = remainder;

                        while(*(++lookAhead) == value)
                        {
                            if(--remainder == 0)
                                return (backTrack + 1); // success
                        }
                        tailSize -= (prevRemainder - remainder);
                    }
                    else
                        return last; // failure
                }

                // lookAhead here is always pointing to the element of the last mismatch.
            }
        }

        return last; // failure
    }


    /// search_n
    ///
    /// Returns: The first iterator i in the range [first, last count) such that 
    /// for any nonnegative integer n less than count the following corresponding 
    /// conditions hold: *(i + n) == value, pred(*(i + n),value) != false. 
    /// Returns last if no such iterator is found.
    ///
    /// Complexity: At most '(last1 - first1) * count' applications of the corresponding predicate.
    ///
    template <typename ForwardIterator, typename Size, typename T>
    ForwardIterator
    search_n(ForwardIterator first, ForwardIterator last, Size count, const T& value)
    {
        typedef typename eastl::iterator_traits<ForwardIterator>::iterator_category IC;
        return eastl::search_n_impl(first, last, count, value, IC());
    }


    /// binary_search
    ///
    /// Returns: true if there is an iterator i in the range [first last) that 
    /// satisfies the corresponding conditions: !(*i < value) && !(value < *i).
    ///
    /// Complexity: At most 'log(last - first) + 2' comparisons.
    ///
    /// Note: The reason binary_search returns bool instead of an iterator is
    /// that search_n, lower_bound, or equal_range already return an iterator. 
    /// However, there are arguments that binary_search should return an iterator.
    /// Note that we provide binary_search_i (STL extension) to return an iterator.
    ///
    /// To use search_n to find an item, do this:
    ///     iterator i = search_n(begin, end, 1, value);
    /// To use lower_bound to find an item, do this:
    ///     iterator i = lower_bound(begin, end, value);
    ///     if((i != last) && !(value < *i))
    ///         <use the iterator>
    /// It turns out that the above lower_bound method is as fast as binary_search
    /// would be if it returned an iterator.
    ///
    template <typename ForwardIterator, typename T>
    inline bool
    binary_search(ForwardIterator first, ForwardIterator last, const T& value)
    {
        // To do: This can be made slightly faster by not using lower_bound.
        ForwardIterator i(eastl::lower_bound<ForwardIterator, T>(first, last, value));
        return ((i != last) && !(value < *i)); // Note that we always express value comparisons in terms of < or ==.
    }


    /// binary_search
    ///
    /// Returns: true if there is an iterator i in the range [first last) that 
    /// satisfies the corresponding conditions: compare(*i, value) == false && 
    /// compare(value, *i) == false.
    ///
    /// Complexity: At most 'log(last - first) + 2' comparisons.
    ///
    /// Note: See comments above regarding the bool return value of binary_search.
    ///
    template <typename ForwardIterator, typename T, typename Compare>
    inline bool
    binary_search(ForwardIterator first, ForwardIterator last, const T& value, Compare compare)
    {
        // To do: This can be made slightly faster by not using lower_bound.
        ForwardIterator i(eastl::lower_bound<ForwardIterator, T, Compare>(first, last, value, compare));
        return ((i != last) && !compare(value, *i));
    }


    /// binary_search_i
    ///
    /// Returns: iterator if there is an iterator i in the range [first last) that 
    /// satisfies the corresponding conditions: !(*i < value) && !(value < *i).
    /// Returns last if the value is not found.
    ///
    /// Complexity: At most 'log(last - first) + 2' comparisons.
    ///
    template <typename ForwardIterator, typename T>
    inline ForwardIterator
    binary_search_i(ForwardIterator first, ForwardIterator last, const T& value)
    {
        // To do: This can be made slightly faster by not using lower_bound.
        ForwardIterator i(eastl::lower_bound<ForwardIterator, T>(first, last, value));
        if((i != last) && !(value < *i)) // Note that we always express value comparisons in terms of < or ==.
            return i;
        return last;
    }


    /// binary_search_i
    ///
    /// Returns: iterator if there is an iterator i in the range [first last) that 
    /// satisfies the corresponding conditions: !(*i < value) && !(value < *i).
    /// Returns last if the value is not found.
    ///
    /// Complexity: At most 'log(last - first) + 2' comparisons.
    ///
    template <typename ForwardIterator, typename T, typename Compare>
    inline ForwardIterator
    binary_search_i(ForwardIterator first, ForwardIterator last, const T& value, Compare compare)
    {
        // To do: This can be made slightly faster by not using lower_bound.
        ForwardIterator i(eastl::lower_bound<ForwardIterator, T, Compare>(first, last, value, compare));
        if((i != last) && !compare(value, *i))
            return i;
        return last;
    }


    /// unique
    ///
    /// Given a sorted range, this function removes duplicated items.
    /// Note that if you have a container then you will probably want 
    /// to call erase on the container with the return value if your 
    /// goal is to remove the duplicated items from the container.
    ///
    /// Effects: Eliminates all but the first element from every consecutive 
    /// group of equal elements referred to by the iterator i in the range 
    /// [first, last) for which the following corresponding condition holds:
    /// *i == *(i - 1).
    ///
    /// Returns: The end of the resulting range.
    ///
    /// Complexity: If the range (last - first) is not empty, exactly (last - first)
    /// applications of the corresponding predicate, otherwise no applications of the predicate.
    ///
    /// Example usage:
    ///    vector<int> intArray;
    ///    ...
    ///    intArray.erase(unique(intArray.begin(), intArray.end()), intArray.end());
    ///
    template <typename ForwardIterator>
    ForwardIterator unique(ForwardIterator first, ForwardIterator last)
    {
        first = eastl::adjacent_find<ForwardIterator>(first, last);

        if(first != last) // We expect that there are duplicated items, else the user wouldn't be calling this function.
        {
            ForwardIterator dest(first);
            
            for(++first; first != last; ++first)
            {
                if(!(*dest == *first)) // Note that we always express value comparisons in terms of < or ==.
                    *++dest = *first;
            }
            return ++dest;
        }
        return last;
    }


    /// unique
    ///
    /// Given a sorted range, this function removes duplicated items.
    /// Note that if you have a container then you will probably want 
    /// to call erase on the container with the return value if your 
    /// goal is to remove the duplicated items from the container.
    ///
    /// Effects: Eliminates all but the first element from every consecutive 
    /// group of equal elements referred to by the iterator i in the range 
    /// [first, last) for which the following corresponding condition holds:
    /// predicate(*i, *(i - 1)) != false.
    ///
    /// Returns: The end of the resulting range.
    ///
    /// Complexity: If the range (last - first) is not empty, exactly (last - first)
    /// applications of the corresponding predicate, otherwise no applications of the predicate.
    ///
    template <typename ForwardIterator, typename BinaryPredicate>
    ForwardIterator unique(ForwardIterator first, ForwardIterator last, BinaryPredicate predicate)
    {
        first = eastl::adjacent_find<ForwardIterator, BinaryPredicate>(first, last, predicate);

        if(first != last) // We expect that there are duplicated items, else the user wouldn't be calling this function.
        {
            ForwardIterator dest(first);
            
            for(++first; first != last; ++first)
            {
                if(!predicate(*dest, *first))
                    *++dest = *first;
            }
            return ++dest;
        }
        return last;
    }



    // find_end
    //
    // We provide two versions here, one for a bidirectional iterators and one for
    // regular forward iterators. Given that we are searching backward, it's a bit 
    // more efficient if we can use backwards iteration to implement our search, 
    // though this requires an iterator that can be reversed.
    //
    template <typename ForwardIterator1, typename ForwardIterator2>
    ForwardIterator1
    find_end_impl(ForwardIterator1 first1, ForwardIterator1 last1,
                  ForwardIterator2 first2, ForwardIterator2 last2,
                  EASTL_ITC_NS::forward_iterator_tag, EASTL_ITC_NS::forward_iterator_tag)
    {
        if(first2 != last2) // We have to do this check because the search algorithm below will return first1 (and not last1) if the first2/last2 range is empty.
        {
            for(ForwardIterator1 result(last1); ; )
            {
                const ForwardIterator1 resultNext(eastl::search(first1, last1, first2, last2));

                if(resultNext != last1) // If another sequence was found...
                {
                    first1 = result = resultNext;
                    ++first1;
                }
                else
                    return result;
            }
        }
        return last1;
    }

    template <typename BidirectionalIterator1, typename BidirectionalIterator2>
    BidirectionalIterator1
    find_end_impl(BidirectionalIterator1 first1, BidirectionalIterator1 last1,
                  BidirectionalIterator2 first2, BidirectionalIterator2 last2,
                  EASTL_ITC_NS::bidirectional_iterator_tag, EASTL_ITC_NS::bidirectional_iterator_tag)
    {
        typedef eastl::reverse_iterator<BidirectionalIterator1> reverse_iterator1;
        typedef eastl::reverse_iterator<BidirectionalIterator2> reverse_iterator2;

        reverse_iterator1 rresult(eastl::search(reverse_iterator1(last1), reverse_iterator1(first1), 
                                                reverse_iterator2(last2), reverse_iterator2(first2)));
        if(rresult.base() != first1) // If we found something...
        {
            BidirectionalIterator1 result(rresult.base());

            eastl::advance(result, -eastl::distance(first2, last2)); // We have an opportunity to optimize this, as the 
            return result;                                           // search function already calculates this distance.
        }
        return last1;
    }

    /// find_end
    ///
    /// Finds the last occurrence of the second sequence in the first sequence.
    /// As such, this function is much like the C string function strrstr and it 
    /// is also the same as a reversed version of 'search'. It is called find_end
    /// instead of the possibly more consistent search_end simply because the C++
    /// standard algorithms have such naming.
    ///
    /// Returns an iterator between first1 and last1 if the sequence is found.
    /// returns last1 (the end of the first seqence) if the sequence is not found.
    ///
    template <typename ForwardIterator1, typename ForwardIterator2>
    inline ForwardIterator1
    find_end(ForwardIterator1 first1, ForwardIterator1 last1,
             ForwardIterator2 first2, ForwardIterator2 last2)
    {
        typedef typename eastl::iterator_traits<ForwardIterator1>::iterator_category IC1;
        typedef typename eastl::iterator_traits<ForwardIterator2>::iterator_category IC2;

        return eastl::find_end_impl(first1, last1, first2, last2, IC1(), IC2());
    }




    // To consider: Fold the predicate and non-predicate versions of 
    //              this algorithm into a single function.
    template <typename ForwardIterator1, typename ForwardIterator2, typename BinaryPredicate>
    ForwardIterator1
    find_end_impl(ForwardIterator1 first1, ForwardIterator1 last1,
                  ForwardIterator2 first2, ForwardIterator2 last2,
                  BinaryPredicate predicate,
                  EASTL_ITC_NS::forward_iterator_tag, EASTL_ITC_NS::forward_iterator_tag)
    {
        if(first2 != last2) // We have to do this check because the search algorithm below will return first1 (and not last1) if the first2/last2 range is empty.
        {
            for(ForwardIterator1 result = last1; ; )
            {
                const ForwardIterator1 resultNext(eastl::search<ForwardIterator1, ForwardIterator2, BinaryPredicate>(first1, last1, first2, last2, predicate));

                if(resultNext != last1) // If another sequence was found...
                {
                    first1 = result = resultNext;
                    ++first1;
                }
                else
                    return result;
            }
        }
        return last1;
    }

    template <typename BidirectionalIterator1, typename BidirectionalIterator2, typename BinaryPredicate>
    BidirectionalIterator1
    find_end_impl(BidirectionalIterator1 first1, BidirectionalIterator1 last1,
                  BidirectionalIterator2 first2, BidirectionalIterator2 last2,
                  BinaryPredicate predicate, 
                  EASTL_ITC_NS::bidirectional_iterator_tag, EASTL_ITC_NS::bidirectional_iterator_tag)
    {
        typedef eastl::reverse_iterator<BidirectionalIterator1> reverse_iterator1;
        typedef eastl::reverse_iterator<BidirectionalIterator2> reverse_iterator2;

        reverse_iterator1 rresult(eastl::search<reverse_iterator1, reverse_iterator2, BinaryPredicate>
                                               (reverse_iterator1(last1), reverse_iterator1(first1), 
                                                reverse_iterator2(last2), reverse_iterator2(first2),
                                                predicate));
        if(rresult.base() != first1) // If we found something...
        {
            BidirectionalIterator1 result(rresult.base());
            eastl::advance(result, -eastl::distance(first2, last2));
            return result;
        }
        return last1;
    }


    /// find_end
    ///
    /// Effects: Finds a subsequence of equal values in a sequence.
    ///
    /// Returns: The last iterator i in the range [first1, last1 - (last2 - first2))
    /// such that for any nonnegative integer n < (last2 - first2), the following 
    /// corresponding conditions hold: pred(*(i+n),*(first2+n)) != false. Returns 
    /// last1 if no such iterator is found.
    ///
    /// Complexity: At most (last2 - first2) * (last1 - first1 - (last2 - first2) + 1)
    /// applications of the corresponding predicate.
    ///
    template <typename ForwardIterator1, typename ForwardIterator2, typename BinaryPredicate>
    inline ForwardIterator1
    find_end(ForwardIterator1 first1, ForwardIterator1 last1,
             ForwardIterator2 first2, ForwardIterator2 last2,
             BinaryPredicate predicate)
    {
        typedef typename eastl::iterator_traits<ForwardIterator1>::iterator_category IC1;
        typedef typename eastl::iterator_traits<ForwardIterator2>::iterator_category IC2;

        return eastl::find_end_impl<ForwardIterator1, ForwardIterator2, BinaryPredicate>
                                   (first1, last1, first2, last2, predicate, IC1(), IC2());
    }



    /// set_difference
    ///
    /// set_difference iterates over both input ranges and copies elements present 
    /// in the first range but not the second to the output range.
    ///
    /// Effects: Copies the elements of the range [first1, last1) which are not 
    /// present in the range [first2, last2) to the range beginning at result. 
    /// The elements in the constructed range are sorted.
    /// 
    /// Requires: The input ranges must be sorted.
    /// Requires: The output range shall not overlap with either of the original ranges.
    /// 
    /// Returns: The end of the output range.
    /// 
    /// Complexity: At most (2 * ((last1 - first1) + (last2 - first2)) - 1) comparisons.
    ///
    template <typename InputIterator1, typename InputIterator2, typename OutputIterator>
    OutputIterator set_difference(InputIterator1 first1, InputIterator1 last1,
                                  InputIterator2 first2, InputIterator2 last2, 
                                  OutputIterator result) 
    {
        while((first1 != last1) && (first2 != last2))
        {
            if(*first1 < *first2) 
            {
                *result = *first1;
                ++first1;
                ++result;
            }
            else if(*first2 < *first1)
                ++first2;
            else 
            {
                ++first1;
                ++first2;
            }
        }

        return eastl::copy(first1, last1, result);
    }


    template <typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Compare>
    OutputIterator set_difference(InputIterator1 first1, InputIterator1 last1,
                                  InputIterator2 first2, InputIterator2 last2, 
                                  OutputIterator result, Compare compare) 
    {
        while((first1 != last1) && (first2 != last2))
        {
            if(compare(*first1, *first2)) 
            {
                EASTL_VALIDATE_COMPARE(!compare(*first2, *first1)); // Validate that the compare function is sane.
                *result = *first1;
                ++first1;
                ++result;
            }
            else if(compare(*first2, *first1))
            {
                EASTL_VALIDATE_COMPARE(!compare(*first1, *first2)); // Validate that the compare function is sane.
                ++first2;
            }
            else 
            {
                ++first1;
                ++first2;
            }
        }

        return eastl::copy(first1, last1, result);
    }

} // namespace eastl



#endif // Header include guard















