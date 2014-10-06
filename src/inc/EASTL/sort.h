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
// EASTL/sort.h
// Written by Paul Pedriana - 2005.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// This file implements sorting algorithms. Some of these are equivalent to 
// std C++ sorting algorithms, while others don't have equivalents in the 
// C++ standard. We implement the following sorting algorithms:
//    is_sorted
//    sort                  The implementation of this is simply mapped to quick_sort.
//    quick_sort
//    partial_sort
//    insertion_sort
//    shell_sort
//    heap_sort
//    stable_sort           The implementation of this is simply mapped to merge_sort.
//    merge
//    merge_sort
//    merge_sort_buffer
//    nth_element
//    radix_sort            Found in sort_extra.h.
//    comb_sort             Found in sort_extra.h.
//    bubble_sort           Found in sort_extra.h.
//    selection_sort        Found in sort_extra.h.
//    shaker_sort           Found in sort_extra.h.
//    bucket_sort           Found in sort_extra.h.
//
// Additional sorting and related algorithms we may want to implement:
//    partial_sort_copy     This would be like the std STL version.
//    paritition            This would be like the std STL version. This is not categorized as a sort routine by the language standard.
//    stable_partition      This would be like the std STL version.
//    counting_sort         Maybe we don't want to implement this.
//
//////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_SORT_H
#define EASTL_SORT_H



#include <EASTL/internal/config.h>
#include <EASTL/iterator.h>
#include <EASTL/memory.h>
#include <EASTL/algorithm.h>
#include <EASTL/functional.h>
#include <EASTL/heap.h>
#include <EASTL/allocator.h>
#include <EASTL/memory.h>



namespace eastl
{

    /// is_sorted
    ///
    /// Returns true if the range [first, last) is sorted.
    /// An empty range is considered to be sorted.
    /// To test if a range is reverse-sorted, use 'greater' as the comparison 
    /// instead of 'less'.
    ///
    /// Example usage:
    ///    vector<int> intArray;
    ///    bool bIsSorted        = is_sorted(intArray.begin(), intArray.end());
    ///    bool bIsReverseSorted = is_sorted(intArray.begin(), intArray.end(), greater<int>());
    ///
    template <typename ForwardIterator, typename StrictWeakOrdering>
    bool is_sorted(ForwardIterator first, ForwardIterator last, StrictWeakOrdering compare)
    {
        if(first != last)
        {
            ForwardIterator current = first;

            for(++current; current != last; first = current, ++current)
            {
                if(compare(*current, *first))
                {
                    EASTL_VALIDATE_COMPARE(!compare(*first, *current)); // Validate that the compare function is sane.
                    return false;
                }
            }
        }
        return true;
    }

    template <typename ForwardIterator>
    inline bool is_sorted(ForwardIterator first, ForwardIterator last)
    {
        typedef eastl::less<typename eastl::iterator_traits<ForwardIterator>::value_type> Less;

        return eastl::is_sorted<ForwardIterator, Less>(first, last, Less());
    }



    /// merge
    ///
    /// This function merges two sorted input sorted ranges into a result sorted range.
    /// This merge is stable in that no element from the first range will be changed
    /// in order relative to other elements from the first range.
    ///
    template <typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Compare>
    OutputIterator merge(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, OutputIterator result, Compare compare)
    {
        while((first1 != last1) && (first2 != last2))
        {
            if(compare(*first2, *first1))
            {
                EASTL_VALIDATE_COMPARE(!compare(*first1, *first2)); // Validate that the compare function is sane.
                *result = *first2;
                ++first2;
            }
            else
            {
                *result = *first1;
                ++first1;
            }
            ++result;
        }

        return eastl::copy(first2, last2, eastl::copy(first1, last1, result));
    }

    template <typename InputIterator1, typename InputIterator2, typename OutputIterator>
    inline OutputIterator merge(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, OutputIterator result)
    {
        typedef eastl::less<typename eastl::iterator_traits<InputIterator1>::value_type> Less;

        return eastl::merge<InputIterator1, InputIterator2, OutputIterator, Less>
                           (first1, last1, first2, last2, result, Less());
    }



    /// insertion_sort
    ///
    /// Implements the InsertionSort algorithm. 
    ///
    template <typename BidirectionalIterator, typename StrictWeakOrdering>
    void insertion_sort(BidirectionalIterator first, BidirectionalIterator last, StrictWeakOrdering compare)
    {
        typedef typename eastl::iterator_traits<BidirectionalIterator>::value_type value_type;

        if(first != last)
        {
            BidirectionalIterator iCurrent, iNext, iSorted = first;

            for(++iSorted; iSorted != last; ++iSorted)
            {
                const value_type temp(*iSorted);

                iNext = iCurrent = iSorted;

                for(--iCurrent; (iNext != first) && compare(temp, *iCurrent); --iNext, --iCurrent)
                {
                    EASTL_VALIDATE_COMPARE(!compare(*iCurrent, temp)); // Validate that the compare function is sane.
                    *iNext = *iCurrent;
                }

                *iNext = temp;
            }
        }
    } // insertion_sort



    template <typename BidirectionalIterator>
    void insertion_sort(BidirectionalIterator first, BidirectionalIterator last)
    {
        typedef typename eastl::iterator_traits<BidirectionalIterator>::value_type value_type;

        if(first != last)
        {
            BidirectionalIterator iCurrent, iNext, iSorted = first;

            for(++iSorted; iSorted != last; ++iSorted)
            {
                const value_type temp(*iSorted);

                iNext = iCurrent = iSorted;

                for(--iCurrent; (iNext != first) && (temp < *iCurrent); --iNext, --iCurrent)
                {
                    EASTL_VALIDATE_COMPARE(!(*iCurrent < temp)); // Validate that the compare function is sane.
                    *iNext = *iCurrent;
                }

                *iNext = temp;
            }
        }
    } // insertion_sort


    #if 0 /*
    // STLPort-like variation of insertion_sort. Doesn't seem to run quite as fast for small runs.
    //
    template <typename RandomAccessIterator, typename Compare>
    void insertion_sort(RandomAccessIterator first, RandomAccessIterator last, Compare compare)
    {
        if(first != last)
        {
            for(RandomAccessIterator i = first + 1; i != last; ++i)
            {
                const typename eastl::iterator_traits<RandomAccessIterator>::value_type value(*i);

                if(compare(value, *first))
                {
                    EASTL_VALIDATE_COMPARE(!compare(*first, value)); // Validate that the compare function is sane.
                    eastl::copy_backward(first, i, i + 1);
                    *first = value;
                }
                else
                {
                    RandomAccessIterator end(i), prev(i);

                    for(--prev; compare(value, *prev); --end, --prev)
                    {
                        EASTL_VALIDATE_COMPARE(!compare(*prev, value)); // Validate that the compare function is sane.
                        *end = *prev;
                    }

                    *end = value;
                }
            }
        }
    }


    // STLPort-like variation of insertion_sort. Doesn't seem to run quite as fast for small runs.
    //
    template <typename RandomAccessIterator>
    void insertion_sort(RandomAccessIterator first, RandomAccessIterator last)
    {
        if(first != last)
        {
            for(RandomAccessIterator i = first + 1; i != last; ++i)
            {
                const typename eastl::iterator_traits<RandomAccessIterator>::value_type value(*i);

                if(value < *first)
                {
                    EASTL_VALIDATE_COMPARE(!(*first < value)); // Validate that the compare function is sane.
                    eastl::copy_backward(first, i, i + 1);
                    *first = value;
                }
                else
                {
                    RandomAccessIterator end(i), prev(i);

                    for(--prev; value < *prev; --end, --prev)
                    {
                        EASTL_VALIDATE_COMPARE(!(*prev < value)); // Validate that the compare function is sane.
                        *end = *prev;
                    }

                    *end = value;
                }
            }
        }
    } */
    #endif


    /// shell_sort
    ///
    /// Implements the ShellSort algorithm. This algorithm is a serious algorithm for larger 
    /// data sets, as reported by Sedgewick in his discussions on QuickSort. Note that shell_sort
    /// requires a random access iterator, which usually means an array (eg. vector, deque).
    /// ShellSort has good performance with presorted sequences.
    /// The term "shell" derives from the name of the inventor, David Shell.
    ///
    /// To consider: Allow the user to specify the "h-sequence" array.
    ///
    template <typename RandomAccessIterator, typename StrictWeakOrdering>
    void shell_sort(RandomAccessIterator first, RandomAccessIterator last, StrictWeakOrdering compare)
    {
        typedef typename eastl::iterator_traits<RandomAccessIterator>::difference_type difference_type;

        // We use the Knuth 'h' sequence below, as it is easy to calculate at runtime. 
        // However, possibly we are better off using a different sequence based on a table.
        // One such sequence which averages slightly better than Knuth is:
        //    1, 5, 19, 41, 109, 209, 505, 929, 2161, 3905, 8929, 16001, 36289, 
        //    64769, 146305, 260609, 587521, 1045505, 2354689, 4188161, 9427969, 16764929

        if(first != last)
        {
            RandomAccessIterator iCurrent, iBack, iSorted, iInsertFirst;
            difference_type      nSize  = last - first;
            difference_type      nSpace = 1; // nSpace is the 'h' value of the ShellSort algorithm.

            while(nSpace < nSize)
                nSpace = (nSpace * 3) + 1; // This is the Knuth 'h' sequence: 1, 4, 13, 40, 121, 364, 1093, 3280, 9841, 29524, 88573, 265720, 797161, 2391484, 7174453, 21523360, 64570081, 193710244, 

            for(nSpace = (nSpace - 1) / 3; nSpace >= 1; nSpace = (nSpace - 1) / 3)  // Integer division is less than ideal.
            {
                for(difference_type i = 0; i < nSpace; i++)
                {
                    iInsertFirst = first + i;

                    for(iSorted = iInsertFirst + nSpace; iSorted < last; iSorted += nSpace)
                    {
                        iBack = iCurrent = iSorted;
                        
                        for(iBack -= nSpace; (iCurrent != iInsertFirst) && compare(*iCurrent, *iBack); iCurrent = iBack, iBack -= nSpace)
                        {
                            EASTL_VALIDATE_COMPARE(!compare(*iBack, *iCurrent)); // Validate that the compare function is sane.
                            eastl::iter_swap(iCurrent, iBack);
                        }
                    }
                }
            }
        }
    } // shell_sort

    template <typename RandomAccessIterator>
    inline void shell_sort(RandomAccessIterator first, RandomAccessIterator last)
    {
        typedef eastl::less<typename eastl::iterator_traits<RandomAccessIterator>::value_type> Less;

        eastl::shell_sort<RandomAccessIterator, Less>(first, last, Less());
    }



    /// heap_sort
    ///
    /// Implements the HeapSort algorithm. 
    /// Note that heap_sort requires a random access iterator, which usually means 
    /// an array (eg. vector, deque).
    ///
    template <typename RandomAccessIterator, typename StrictWeakOrdering>
    void heap_sort(RandomAccessIterator first, RandomAccessIterator last, StrictWeakOrdering compare)
    {
        // We simply call our heap algorithms to do the work for us.
        eastl::make_heap<RandomAccessIterator, StrictWeakOrdering>(first, last, compare);
        eastl::sort_heap<RandomAccessIterator, StrictWeakOrdering>(first, last, compare);
    }

    template <typename RandomAccessIterator>
    inline void heap_sort(RandomAccessIterator first, RandomAccessIterator last)
    {
        typedef eastl::less<typename eastl::iterator_traits<RandomAccessIterator>::value_type> Less;

        eastl::heap_sort<RandomAccessIterator, Less>(first, last, Less());
    }




    /// merge_sort_buffer
    ///
    /// Implements the MergeSort algorithm with a user-supplied buffer.
    /// The input buffer must be able to hold a number of items equal to 'last - first'.
    /// Note that merge_sort_buffer requires a random access iterator, which usually means 
    /// an array (eg. vector, deque).
    ///
    
    // For reference, the following is the simple version, before inlining one level 
    // of recursion and eliminating the copy:
    //
    //template <typename RandomAccessIterator, typename T, typename StrictWeakOrdering>
    //void merge_sort_buffer(RandomAccessIterator first, RandomAccessIterator last, T* pBuffer, StrictWeakOrdering compare)
    //{
    //    typedef typename eastl::iterator_traits<RandomAccessIterator>::difference_type difference_type;
    //
    //    const difference_type nCount = last - first;
    //
    //    if(nCount > 1)
    //    {
    //        const difference_type nMid = nCount / 2;
    //
    //        eastl::merge_sort_buffer<RandomAccessIterator, T, StrictWeakOrdering>
    //                                (first,        first + nMid, pBuffer, compare);
    //        eastl::merge_sort_buffer<RandomAccessIterator, T, StrictWeakOrdering>
    //                                (first + nMid, last        , pBuffer, compare);
    //        eastl::copy(first, last, pBuffer);
    //        eastl::merge<T*, T*, RandomAccessIterator, StrictWeakOrdering>
    //                    (pBuffer, pBuffer + nMid, pBuffer + nMid, pBuffer + nCount, first, compare);
    //    }
    //}
    
    template <typename RandomAccessIterator, typename T, typename StrictWeakOrdering>
    void merge_sort_buffer(RandomAccessIterator first, RandomAccessIterator last, T* pBuffer, StrictWeakOrdering compare)
    {
        typedef typename eastl::iterator_traits<RandomAccessIterator>::difference_type difference_type;
        const difference_type nCount = last - first;

        if(nCount > 1)
        {
            const difference_type nMid = nCount / 2;
            RandomAccessIterator half = first + nMid;
 
            if(nMid > 1)
            {
                const difference_type nQ1(nMid / 2);
                RandomAccessIterator  part(first + nQ1);

                eastl::merge_sort_buffer<RandomAccessIterator, T, StrictWeakOrdering>(first, part, pBuffer,       compare);
                eastl::merge_sort_buffer<RandomAccessIterator, T, StrictWeakOrdering>(part,  half, pBuffer + nQ1, compare);
                eastl::merge<RandomAccessIterator, RandomAccessIterator, T*, StrictWeakOrdering>
                            (first, part, part, half, pBuffer, compare);
            }
            else
                *pBuffer = *first;
 
            if((nCount - nMid) > 1)
            {
                const difference_type nQ3((nMid + nCount) / 2);
                RandomAccessIterator  part(first + nQ3);

                eastl::merge_sort_buffer<RandomAccessIterator, T, StrictWeakOrdering>(half, part, pBuffer + nMid, compare);
                eastl::merge_sort_buffer<RandomAccessIterator, T, StrictWeakOrdering>(part, last, pBuffer + nQ3,  compare);
                eastl::merge<RandomAccessIterator, RandomAccessIterator, T*, StrictWeakOrdering>
                            (half, part, part, last, pBuffer + nMid, compare);
            }
            else
                *(pBuffer + nMid) = *half;
 
            eastl::merge<T*, T*, RandomAccessIterator, StrictWeakOrdering>
                        (pBuffer, pBuffer + nMid, pBuffer + nMid, pBuffer + nCount, first, compare);
        }
    }

    template <typename RandomAccessIterator, typename T>
    inline void merge_sort_buffer(RandomAccessIterator first, RandomAccessIterator last, T* pBuffer)
    {
        typedef eastl::less<typename eastl::iterator_traits<RandomAccessIterator>::value_type> Less;

        eastl::merge_sort_buffer<RandomAccessIterator, T, Less>(first, last, pBuffer, Less());
    }



    /// merge_sort
    ///
    /// Implements the MergeSort algorithm.
    /// This algorithm allocates memory via the user-supplied allocator. Use merge_sort_buffer
    /// function if you want a version which doesn't allocate memory.
    /// Note that merge_sort requires a random access iterator, which usually means 
    /// an array (eg. vector, deque).
    /// 
    template <typename RandomAccessIterator, typename Allocator, typename StrictWeakOrdering>
    void merge_sort(RandomAccessIterator first, RandomAccessIterator last, Allocator& allocator, StrictWeakOrdering compare)
    {
        typedef typename eastl::iterator_traits<RandomAccessIterator>::difference_type difference_type;
        typedef typename eastl::iterator_traits<RandomAccessIterator>::value_type      value_type;

        const difference_type nCount = last - first;

        if(nCount > 1)
        {
            // We need to allocate an array of nCount value_type objects as a temporary buffer.
            value_type* const pBuffer = (value_type*)allocate_memory(allocator, nCount * sizeof(value_type), EASTL_ALIGN_OF(value_type), 0);
            eastl::uninitialized_fill(pBuffer, pBuffer + nCount, value_type());

            eastl::merge_sort_buffer<RandomAccessIterator, value_type, StrictWeakOrdering>
                                    (first, last, pBuffer, compare);

            eastl::destruct(pBuffer, pBuffer + nCount);
            EASTLFree(allocator, pBuffer, nCount * sizeof(value_type));
        }
    }

    template <typename RandomAccessIterator, typename Allocator>
    inline void merge_sort(RandomAccessIterator first, RandomAccessIterator last, Allocator& allocator)
    {
        typedef eastl::less<typename eastl::iterator_traits<RandomAccessIterator>::value_type> Less;

        eastl::merge_sort<RandomAccessIterator, Allocator, Less>(first, last, allocator, Less());
    }



    /////////////////////////////////////////////////////////////////////
    // quick_sort
    //
    // We do the "introspection sort" variant of quick sort which is now
    // well-known and understood. You can read about this algorithm in
    // many articles on quick sort, but briefly what it does is a median-
    // of-three quick sort whereby the recursion depth is limited to a
    // some value (after which it gives up on quick sort and switches to
    // a heap sort) and whereby after a certain amount of sorting the 
    // algorithm stops doing quick-sort and finishes the sorting via
    // a simple insertion sort.
    /////////////////////////////////////////////////////////////////////

    static const int kQuickSortLimit = 28; // For sorts of random arrays over 100 items, 28 - 32 have been found to be good numbers on VC++/Win32.

    namespace Internal
    {
        template <typename Size>
        inline Size Log2(Size n)
        {
            int i;
            for(i = 0; n; ++i)
                n >>= 1;
            return i - 1;
        }

        // To do: Investigate the speed of this bit-trick version of Log2.
        //        It may work better on some platforms but not others.
        //
        // union FloatUnion {
        //     float    f;
        //     uint32_t i;
        // };
        // 
        // inline uint32_t Log2(uint32_t x)
        // {
        //     const FloatInt32Union u = { x };
        //     return (u.i >> 23) - 127;
        // }
    }


    /// get_partition
    ///
    /// This function takes const T& instead of T because T may have special alignment
    /// requirements and some compilers (e.g. VC++) are don't respect alignment requirements
    /// for function arguments.
    ///
    template <typename RandomAccessIterator, typename T>
    inline RandomAccessIterator get_partition(RandomAccessIterator first, RandomAccessIterator last, const T& pivotValue)
    {
        const T pivotCopy(pivotValue); // Need to make a temporary because the sequence below is mutating.

        for(; ; ++first)
        {
            while(*first < pivotCopy)
            {
                EASTL_VALIDATE_COMPARE(!(pivotCopy < *first)); // Validate that the compare function is sane.
                ++first;
            }
            --last;

            while(pivotCopy < *last)
            {
                EASTL_VALIDATE_COMPARE(!(*last < pivotCopy)); // Validate that the compare function is sane.
                --last;
            }

            if(first >= last) // Random access iterators allow operator >=
                return first;

            eastl::iter_swap(first, last);
        }
    }


    template <typename RandomAccessIterator, typename T, typename Compare>
    inline RandomAccessIterator get_partition(RandomAccessIterator first, RandomAccessIterator last, const T& pivotValue, Compare compare)
    {
        const T pivotCopy(pivotValue); // Need to make a temporary because the sequence below is mutating.

        for(; ; ++first)
        {
            while(compare(*first, pivotCopy))
            {
                EASTL_VALIDATE_COMPARE(!compare(pivotCopy, *first)); // Validate that the compare function is sane.
                ++first;
            }
            --last;

            while(compare(pivotCopy, *last))
            {
                EASTL_VALIDATE_COMPARE(!compare(*last, pivotCopy)); // Validate that the compare function is sane.
                --last;
            }

            if(first >= last) // Random access iterators allow operator >=
                return first;

            eastl::iter_swap(first, last);
        }
    }


    namespace Internal
    {
        // This function is used by quick_sort and is not intended to be used by itself. 
        // This is because the implementation below makes an assumption about the input
        // data that quick_sort satisfies but arbitrary data may not.
        // There is a standalone insertion_sort function. 
        template <typename RandomAccessIterator>
        inline void insertion_sort_simple(RandomAccessIterator first, RandomAccessIterator last)
        {
            for(RandomAccessIterator current = first; current != last; ++current)
            {
                typedef typename eastl::iterator_traits<RandomAccessIterator>::value_type value_type;

                RandomAccessIterator end(current), prev(current);
                const value_type     value(*current);

                for(--prev; value < *prev; --end, --prev) // We skip checking for (prev >= first) because quick_sort (our caller) makes this unnecessary.
                {
                    EASTL_VALIDATE_COMPARE(!(*prev < value)); // Validate that the compare function is sane.
                    *end = *prev;
                }

                *end = value;
            }
        }


        // This function is used by quick_sort and is not intended to be used by itself. 
        // This is because the implementation below makes an assumption about the input
        // data that quick_sort satisfies but arbitrary data may not.
        // There is a standalone insertion_sort function. 
        template <typename RandomAccessIterator, typename Compare>
        inline void insertion_sort_simple(RandomAccessIterator first, RandomAccessIterator last, Compare compare)
        {
            for(RandomAccessIterator current = first; current != last; ++current)
            {
                typedef typename eastl::iterator_traits<RandomAccessIterator>::value_type value_type;

                RandomAccessIterator end(current), prev(current);
                const value_type     value(*current);

                for(--prev; compare(value, *prev); --end, --prev) // We skip checking for (prev >= first) because quick_sort (our caller) makes this unnecessary.
                {
                    EASTL_VALIDATE_COMPARE(!compare(*prev, value)); // Validate that the compare function is sane.
                    *end = *prev;
                }

                *end = value;
            }
        }
    } // namespace Internal


    template <typename RandomAccessIterator>
    inline void partial_sort(RandomAccessIterator first, RandomAccessIterator middle, RandomAccessIterator last)
    {
        typedef typename eastl::iterator_traits<RandomAccessIterator>::difference_type difference_type;
        typedef typename eastl::iterator_traits<RandomAccessIterator>::value_type      value_type;

        eastl::make_heap<RandomAccessIterator>(first, middle);

        for(RandomAccessIterator i = middle; i < last; ++i)
        {
            if(*i < *first)
            {
                EASTL_VALIDATE_COMPARE(!(*first < *i)); // Validate that the compare function is sane.
                const value_type temp(*i);
                *i = *first;
                eastl::adjust_heap<RandomAccessIterator, difference_type, value_type>
                                  (first, difference_type(0), difference_type(middle - first), difference_type(0), temp);
            }
        }

        eastl::sort_heap<RandomAccessIterator>(first, middle);
    }


    template <typename RandomAccessIterator, typename Compare>
    inline void partial_sort(RandomAccessIterator first, RandomAccessIterator middle, RandomAccessIterator last, Compare compare)
    {
        typedef typename eastl::iterator_traits<RandomAccessIterator>::difference_type difference_type;
        typedef typename eastl::iterator_traits<RandomAccessIterator>::value_type      value_type;

        eastl::make_heap<RandomAccessIterator, Compare>(first, middle, compare);

        for(RandomAccessIterator i = middle; i < last; ++i)
        {
            if(compare(*i, *first))
            {
                EASTL_VALIDATE_COMPARE(!compare(*first, *i)); // Validate that the compare function is sane.
                const value_type temp(*i);
                *i = *first;
                eastl::adjust_heap<RandomAccessIterator, difference_type, value_type, Compare>
                                  (first, difference_type(0), difference_type(middle - first), difference_type(0), temp, compare);
            }
        }

        eastl::sort_heap<RandomAccessIterator, Compare>(first, middle, compare);
    }


    template<typename RandomAccessIterator>
    inline void nth_element(RandomAccessIterator first, RandomAccessIterator nth, RandomAccessIterator last)
    {
        typedef typename iterator_traits<RandomAccessIterator>::value_type value_type;

        while((last - first) > 5)
        {
            const value_type           midValue(eastl::median<value_type>(*first, *(first + (last - first) / 2), *(last - 1)));
            const RandomAccessIterator midPos(eastl::get_partition<RandomAccessIterator, value_type>(first, last, midValue));

            if(midPos <= nth)
                first = midPos;
            else
                last = midPos;
        }

        eastl::insertion_sort<RandomAccessIterator>(first, last);
    }


    template<typename RandomAccessIterator, typename Compare>
    inline void nth_element(RandomAccessIterator first, RandomAccessIterator nth, RandomAccessIterator last, Compare compare)
    {
        typedef typename iterator_traits<RandomAccessIterator>::value_type value_type;

        while((last - first) > 5)
        {
            const value_type           midValue(eastl::median<value_type, Compare>(*first, *(first + (last - first) / 2), *(last - 1), compare));
            const RandomAccessIterator midPos(eastl::get_partition<RandomAccessIterator, value_type, Compare>(first, last, midValue, compare));

            if(midPos <= nth)
                first = midPos;
            else
                last = midPos;
        }

        eastl::insertion_sort<RandomAccessIterator, Compare>(first, last, compare);
    }


    template <typename RandomAccessIterator, typename Size>
    inline void quick_sort_impl(RandomAccessIterator first, RandomAccessIterator last, Size kRecursionCount)
    {
        typedef typename iterator_traits<RandomAccessIterator>::value_type value_type;

        while(((last - first) > kQuickSortLimit) && (kRecursionCount > 0))
        {
            const RandomAccessIterator position(eastl::get_partition<RandomAccessIterator, value_type>(first, last, eastl::median<value_type>(*first, *(first + (last - first) / 2), *(last - 1))));

            eastl::quick_sort_impl<RandomAccessIterator, Size>(position, last, --kRecursionCount);
            last = position;
        }

        if(kRecursionCount == 0)
            eastl::partial_sort<RandomAccessIterator>(first, last, last);
    }


    template <typename RandomAccessIterator, typename Size, typename Compare>
    inline void quick_sort_impl(RandomAccessIterator first, RandomAccessIterator last, Size kRecursionCount, Compare compare)
    {
        typedef typename iterator_traits<RandomAccessIterator>::value_type value_type;

        while(((last - first) > kQuickSortLimit) && (kRecursionCount > 0))
        {
            const RandomAccessIterator position(eastl::get_partition<RandomAccessIterator, value_type, Compare>(first, last, eastl::median<value_type, Compare>(*first, *(first + (last - first) / 2), *(last - 1), compare), compare));

            eastl::quick_sort_impl<RandomAccessIterator, Size, Compare>(position, last, --kRecursionCount, compare);
            last = position;
        }

        if(kRecursionCount == 0)
            eastl::partial_sort<RandomAccessIterator, Compare>(first, last, last, compare);
    }


    /// quick_sort
    ///
    /// quick_sort sorts the elements in [first, last) into ascending order, 
    /// meaning that if i and j are any two valid iterators in [first, last) 
    /// such that i precedes j, then *j is not less than *i. quick_sort is not 
    /// guaranteed to be stable. That is, suppose that *i and *j are equivalent: 
    /// neither one is less than the other. It is not guaranteed that the 
    /// relative order of these two elements will be preserved by sort.
    ///
    /// We implement the "introspective" variation of quick-sort. This is 
    /// considered to be the best general-purpose variant, as it avoids 
    /// worst-case behaviour and optimizes the final sorting stage by 
    /// switching to an insertion sort.
    ///
    template <typename RandomAccessIterator>
    void quick_sort(RandomAccessIterator first, RandomAccessIterator last)
    {
        typedef typename eastl::iterator_traits<RandomAccessIterator>::difference_type difference_type;

        if(first != last)
        {
            eastl::quick_sort_impl<RandomAccessIterator, difference_type>(first, last, 2 * Internal::Log2(last - first));

            if((last - first) > (difference_type)kQuickSortLimit)
            {
                eastl::insertion_sort<RandomAccessIterator>(first, first + kQuickSortLimit);
                eastl::Internal::insertion_sort_simple<RandomAccessIterator>(first + kQuickSortLimit, last);
            }
            else
                eastl::insertion_sort<RandomAccessIterator>(first, last);
        }
    }


    template <typename RandomAccessIterator, typename Compare>
    void quick_sort(RandomAccessIterator first, RandomAccessIterator last, Compare compare)
    {
        typedef typename eastl::iterator_traits<RandomAccessIterator>::difference_type difference_type;

        if(first != last)
        {
            eastl::quick_sort_impl<RandomAccessIterator, difference_type, Compare>(first, last, 2 * Internal::Log2(last - first), compare);

            if((last - first) > (difference_type)kQuickSortLimit)
            {
                eastl::insertion_sort<RandomAccessIterator, Compare>(first, first + kQuickSortLimit, compare);
                eastl::Internal::insertion_sort_simple<RandomAccessIterator, Compare>(first + kQuickSortLimit, last, compare);
            }
            else
                eastl::insertion_sort<RandomAccessIterator, Compare>(first, last, compare);
        }
    }



    /// sort
    /// 
    /// We simply use quick_sort. See quick_sort for details.
    ///
    template <typename RandomAccessIterator>
    inline void sort(RandomAccessIterator first, RandomAccessIterator last)
    {
        eastl::quick_sort<RandomAccessIterator>(first, last);
    }

    template <typename RandomAccessIterator, typename Compare>
    inline void sort(RandomAccessIterator first, RandomAccessIterator last, Compare compare)
    {
        eastl::quick_sort<RandomAccessIterator, Compare>(first, last, compare);
    }



    /// stable_sort
    /// 
    /// We simply use merge_sort. See merge_sort for details.
    /// Beware that the used merge_sort -- and thus stable_sort -- allocates 
    /// memory during execution. Try using merge_sort_buffer if you want
    /// to avoid memory allocation.
    /// 
    template <typename RandomAccessIterator, typename StrictWeakOrdering>
    void stable_sort(RandomAccessIterator first, RandomAccessIterator last, StrictWeakOrdering compare)
    {
        eastl::merge_sort<RandomAccessIterator, EASTLAllocatorType, StrictWeakOrdering>
                         (first, last, *get_default_allocator(0), compare);
    }

    template <typename RandomAccessIterator>
    void stable_sort(RandomAccessIterator first, RandomAccessIterator last)
    {
        eastl::merge_sort<RandomAccessIterator, EASTLAllocatorType>
                         (first, last, *get_default_allocator(0));
    }

    template <typename RandomAccessIterator, typename Allocator, typename StrictWeakOrdering>
    void stable_sort(RandomAccessIterator first, RandomAccessIterator last, Allocator& allocator, StrictWeakOrdering compare)
    {
        eastl::merge_sort<RandomAccessIterator, Allocator, StrictWeakOrdering>(first, last, allocator, compare);
    }

    // This is not defined because it would cause compiler errors due to conflicts with a version above. 
    //template <typename RandomAccessIterator, typename Allocator>
    //void stable_sort(RandomAccessIterator first, RandomAccessIterator last, Allocator& allocator)
    //{
    //    eastl::merge_sort<RandomAccessIterator, Allocator>(first, last, allocator);
    //}

} // namespace eastl


#endif // Header include guard

















