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
// EASTL/sort_extra.h
// Written by Paul Pedriana - 2005
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// This file implements additional sort algorithms beyond the basic set.
// Included here are:
//    radix_sort
//    comb_sort
//    bubble_sort
//    selection_sort
//    shaker_sort
//    bucket_sort
//
//////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_SORT_EXTRA_H
#define EASTL_SORT_EXTRA_H

#include <EASTL/internal/config.h>
#include <EASTL/iterator.h>
#include <EASTL/algorithm.h>
#include <EASTL/functional.h>
#include <EASTL/heap.h>
#include <EASTL/allocator.h>

namespace eastl
{

    /// extract_radix_key
    ///
    /// Default radix sort integer value reader. It expects the sorted elements 
    /// to have an integer member of type radix_type and of name "mKey". 
    ///
    template <typename Node>
    struct extract_radix_key
    {
        typedef typename Node::radix_type radix_type;

        const radix_type operator()(const Node& x) const
            { return x.mKey; }
    };


    /// radix_sort
    ///
    /// Implements a classic LSD (least significant digit) radix sort.
    /// See http://en.wikipedia.org/wiki/Radix_sort.
    /// To consider: A static linked-list implementation may be faster than the version here.
    ///
    /// Example usage:
    ///     struct Element {
    ///         typedef uint16_t radix_type;
    ///         uint16_t mKey;
    ///         uint16_t mData;
    ///     };
    ///
    ///     Element elementArray[100];
    ///     Element buffer[100];
    ///
    ///     radix_sort<Element*, extract_radix_key<Element> >(elementArray, elementArray + 100, buffer);
    ///
    template <typename RandomAccessIterator, typename ExtractKey>
    void radix_sort_impl(RandomAccessIterator first, RandomAccessIterator last, RandomAccessIterator buffer, ExtractKey extractKey, uint8_t)
    {
        uint32_t EA_PREFIX_ALIGN(16) bucketSize[256] EA_POSTFIX_ALIGN(16);
        uint32_t EA_PREFIX_ALIGN(16) bucketPosition[256] EA_POSTFIX_ALIGN(16);
        RandomAccessIterator temp;
        uint32_t i;

        memset(bucketSize, 0, sizeof(bucketSize));

        for(temp = first; temp != last; ++temp)
            ++bucketSize[extractKey(*temp)];

        for(bucketPosition[0] = 0, i = 0; i < 255; i++)
            bucketPosition[i + 1] = bucketPosition[i] + bucketSize[i];

        for(temp = first; temp != last; ++temp)
        {
            const size_t radixByte = extractKey(*temp);
            buffer[bucketPosition[radixByte]++] = *temp;
        }
    }


    template <typename RandomAccessIterator, typename ExtractKey>
    void radix_sort_impl(RandomAccessIterator first, RandomAccessIterator last, RandomAccessIterator buffer, ExtractKey extractKey, uint16_t)
    {
        uint32_t EA_PREFIX_ALIGN(16) bucketSize[256] EA_POSTFIX_ALIGN(16);
        uint32_t EA_PREFIX_ALIGN(16) bucketPosition[256] EA_POSTFIX_ALIGN(16);
        RandomAccessIterator temp;
        uint32_t i;

        // Process byte 0 (least significant byte).
        memset(bucketSize, 0, sizeof(bucketSize));

        for(temp = first; temp != last; ++temp)
            ++bucketSize[extractKey(*temp) & 0xff];

        for(bucketPosition[0] = 0, i = 0; i < 255; i++)
            bucketPosition[i + 1] = bucketPosition[i] + bucketSize[i];

        for(temp = first; temp != last; ++temp)
        {
            const size_t radixByte = extractKey(*temp) & 0xff;
            buffer[bucketPosition[radixByte]++] = *temp;
        }


        // Process byte 1 (second least significant byte).
        memset(bucketSize, 0, sizeof(bucketSize));

        for(temp = buffer, last = buffer + (last - first); temp != last; ++temp)
            ++bucketSize[extractKey(*temp) >> 8];

        for(bucketPosition[0] = 0, i = 0; i < 255; i++)
            bucketPosition[i + 1] = bucketPosition[i] + bucketSize[i];
        
        for(temp = buffer; temp != last; ++temp)
        {
            const size_t radixByte = extractKey(*temp) >> 8;
            first[bucketPosition[radixByte]++] = *temp;
        }
    }


    template <typename RandomAccessIterator, typename ExtractKey, typename IntegerType>
    void radix_sort_impl(RandomAccessIterator first, RandomAccessIterator last, RandomAccessIterator buffer, ExtractKey extractKey, IntegerType)
    {
        uint32_t EA_PREFIX_ALIGN(16) bucketSize[256] EA_POSTFIX_ALIGN(16);
        uint32_t EA_PREFIX_ALIGN(16) bucketPosition[256] EA_POSTFIX_ALIGN(16);
        RandomAccessIterator temp;
        uint32_t i;

        for(uint32_t j = 0; j < (8 * sizeof(IntegerType)); j += 8)
        {
            memset(bucketSize, 0, sizeof(bucketSize));

            for(temp = first; temp != last; ++temp)
                ++bucketSize[(extractKey(*temp) >> j) & 0xff];

            bucketPosition[0] = 0;
            for(i = 0; i < 255; i++)
                bucketPosition[i + 1] = bucketPosition[i] + bucketSize[i];

            for(temp = first; temp != last; ++temp)
            {
                const size_t radixByte = ((extractKey(*temp) >> j) & 0xff);
                buffer[bucketPosition[radixByte]++] = *temp;
            }

            last   = buffer + (last - first);
            temp   = first;
            first  = buffer;
            buffer = temp; 
        }
    }


    template <typename RandomAccessIterator, typename ExtractKey>
    void radix_sort(RandomAccessIterator first, RandomAccessIterator last, RandomAccessIterator buffer)
    {
        radix_sort_impl<RandomAccessIterator>(first, last, buffer, ExtractKey(), typename ExtractKey::radix_type());
    }



    /// comb_sort
    ///
    /// Implements the CombSort algorithm; in particular, implements the CombSort11 variation 
    /// of the CombSort algorithm, based on the reference to '11' in the implementation.
    ///
    /// To consider: Use a comb sort table instead of the '((nSpace * 10) + 3) / 13' expression.
    ///              Ideal tables can be found on the Internet by looking up "comb sort table".
    ///
    template <typename ForwardIterator, typename StrictWeakOrdering>
    void comb_sort(ForwardIterator first, ForwardIterator last, StrictWeakOrdering compare)
    {
        typedef typename eastl::iterator_traits<ForwardIterator>::difference_type difference_type;

        ForwardIterator iCurrent, iNext;
        difference_type length = eastl::distance(first, last);
        difference_type nSpace = length;

        for(bool bSwapped = false; (nSpace > 1) || bSwapped; )
        {
            nSpace = ((nSpace * 10) + 3) / 13; // Integer division is less than ideal.

            if((nSpace == 9) || (nSpace == 10))
                nSpace = 11;

            iCurrent = iNext = first;
            eastl::advance(iNext, nSpace);
            
            for(bSwapped = false; iNext != last; iCurrent++, iNext++)
            {
                if(compare(*iNext, *iCurrent))
                {
                    EASTL_VALIDATE_COMPARE(!compare(*iCurrent, *iNext)); // Validate that the compare function is sane.
                    eastl::iter_swap(iCurrent, iNext);
                    bSwapped = true;
                }
            }
        }
    } // comb_sort

    template <typename ForwardIterator>
    inline void comb_sort(ForwardIterator first, ForwardIterator last)
    {
        typedef eastl::less<typename eastl::iterator_traits<ForwardIterator>::value_type> Less;

        eastl::comb_sort<ForwardIterator, Less>(first, last, Less());
    }




    /// bubble_sort
    ///
    /// Implements the BubbleSort algorithm. This algorithm is only useful for 
    /// small range sizes, such as 10 or less items. You may be better off using
    /// insertion_sort for cases where bubble_sort works.
    ///
    template <typename ForwardIterator, typename StrictWeakOrdering>
    void bubble_sort_impl(ForwardIterator first, ForwardIterator last, StrictWeakOrdering compare, EASTL_ITC_NS::forward_iterator_tag)
    {
        ForwardIterator iCurrent, iNext;

        while(first != last)
        {
            iNext = iCurrent = first;
            
            for(++iNext; iNext != last; iCurrent = iNext, ++iNext) 
            {
                if(compare(*iNext, *iCurrent))
                {
                    EASTL_VALIDATE_COMPARE(!compare(*iCurrent, *iNext)); // Validate that the compare function is sane.
                    eastl::iter_swap(iCurrent, iNext);
                }
            }
            last = iCurrent;
        }
    }

    template <typename BidirectionalIterator, typename StrictWeakOrdering>
    void bubble_sort_impl(BidirectionalIterator first, BidirectionalIterator last, StrictWeakOrdering compare, EASTL_ITC_NS::bidirectional_iterator_tag)
    {
        if(first != last)
        {
            BidirectionalIterator iCurrent, iNext, iLastModified;

            last--;

            while(first != last)
            {
                iLastModified = iNext = iCurrent = first;
                
                for(++iNext; iCurrent != last; iCurrent = iNext, ++iNext)
                {
                    if(compare(*iNext, *iCurrent))
                    {
                        EASTL_VALIDATE_COMPARE(!compare(*iCurrent, *iNext)); // Validate that the compare function is sane.
                        iLastModified = iCurrent;
                        eastl::iter_swap(iCurrent, iNext);
                    }
                }

                last = iLastModified;
            }
        }
    }

    template <typename ForwardIterator, typename StrictWeakOrdering>
    inline void bubble_sort(ForwardIterator first, ForwardIterator last, StrictWeakOrdering compare)
    {
        typedef typename eastl::iterator_traits<ForwardIterator>::iterator_category IC;

        eastl::bubble_sort_impl<ForwardIterator, StrictWeakOrdering>(first, last, compare, IC());
    }

    template <typename ForwardIterator>
    inline void bubble_sort(ForwardIterator first, ForwardIterator last)
    {
        typedef eastl::less<typename eastl::iterator_traits<ForwardIterator>::value_type> Less;
        typedef typename eastl::iterator_traits<ForwardIterator>::iterator_category IC;

        eastl::bubble_sort_impl<ForwardIterator, Less>(first, last, Less(), IC());
    }



    /// selection_sort
    ///
    /// Implements the SelectionSort algorithm.
    ///
    template <typename ForwardIterator, typename StrictWeakOrdering>
    void selection_sort(ForwardIterator first, ForwardIterator last, StrictWeakOrdering compare)
    {
        if(first != last)
        {
            ForwardIterator iCurrent, iMin;

            for(; first != last; ++first)
            {
                iCurrent = first;
                iMin     = iCurrent;

                for(++iCurrent; iCurrent != last; ++iCurrent)
                {
                    if(compare(*iCurrent, *iMin))
                    {
                        EASTL_VALIDATE_COMPARE(!compare(*iMin, *iCurrent)); // Validate that the compare function is sane.
                        iMin = iCurrent;
                    }
                }

                if(first != iMin)
                    eastl::iter_swap(first, iMin);
            }
        }
    } // selection_sort

    template <typename ForwardIterator>
    inline void selection_sort(ForwardIterator first, ForwardIterator last)
    {
        typedef eastl::less<typename eastl::iterator_traits<ForwardIterator>::value_type> Less;

        eastl::selection_sort<ForwardIterator, Less>(first, last, Less());
    }



    /// shaker_sort
    ///
    /// Implements the ShakerSort algorithm, which is a sorting algorithm which 
    /// improves on bubble_sort by sweeping both from left to right and right 
    /// to left, resulting in less iteration.
    ///
    template <typename BidirectionalIterator, typename StrictWeakOrdering>
    void shaker_sort(BidirectionalIterator first, BidirectionalIterator last, StrictWeakOrdering compare)
    {
        if(first != last)
        {
            BidirectionalIterator iCurrent, iNext, iLastModified;

            --last;

            while(first != last)
            {
                iLastModified = first;

                for(iCurrent = first; iCurrent != last; iCurrent = iNext)
                {
                    iNext = iCurrent;
                    ++iNext;

                    if(compare(*iNext, *iCurrent))
                    {
                        EASTL_VALIDATE_COMPARE(!compare(*iCurrent, *iNext)); // Validate that the compare function is sane.
                        iLastModified = iCurrent;
                        eastl::iter_swap(iCurrent, iNext);
                    }
                }

                last = iLastModified;

                if(first != last)
                {
                    for(iCurrent = last; iCurrent != first; iCurrent = iNext)
                    {
                        iNext = iCurrent;
                        --iNext;

                        if(compare(*iCurrent, *iNext))
                        {
                            EASTL_VALIDATE_COMPARE(!compare(*iNext, *iCurrent)); // Validate that the compare function is sane.
                            iLastModified = iCurrent;
                            eastl::iter_swap(iNext, iCurrent);
                        }
                    }
                    first = iLastModified;
                }
            }
        }
    } // shaker_sort

    template <typename BidirectionalIterator>
    inline void shaker_sort(BidirectionalIterator first, BidirectionalIterator last)
    {
        typedef eastl::less<typename eastl::iterator_traits<BidirectionalIterator>::value_type> Less;

        eastl::shaker_sort<BidirectionalIterator, Less>(first, last, Less());
    }



    /// bucket_sort
    ///
    /// Implements the BucketSort algorithm. 
    ///
    /// Example usage:
    ///     int* pArray = new int[1000];
    ///     for(int i = 0; i < 1000; i++)
    ///         pArray[i] = rand() % 32; // Note: The C rand function is a poor random number generator.
    ///     vector<int> intVector[32];
    ///     bucket_sort(pArray, pArray + 1000; intVector);
    ///     delete[] pArray;
    ///
    template <typename T>
    struct hash_use_self
    {
        T operator()(const T& x) const
            { return x; }
    };

    template <typename ForwardIterator, typename ContainerArray, typename HashFunction>
    void bucket_sort(ForwardIterator first, ForwardIterator last, ContainerArray& bucketArray, HashFunction hash /*= hash_use_self*/)
    {
        typedef typename eastl::iterator_traits<ForwardIterator>::difference_type difference_type;

        for(ForwardIterator iInput = first; iInput != last; ++iInput)
            bucketArray[hash(*iInput)].push_back(*iInput);

        for(typename ContainerArray::const_iterator iBucket = bucketArray.begin(); iBucket != bucketArray.end(); ++iBucket)
            first = eastl::copy((*iBucket).begin(), (*iBucket).end(), first);
    }



} // namespace eastl


#endif // Header include guard



















