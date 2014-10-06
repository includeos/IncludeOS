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
// EASTL/fixed_list.h
//
// Copyright (c) 2005, Electronic Arts. All rights reserved.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements a list which uses a fixed size memory pool for its nodes. 
//
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_FIXED_LIST_H
#define EASTL_FIXED_LIST_H


#include <EASTL/list.h>
#include <EASTL/internal/fixed_pool.h>
#include <EASTL/sort.h>


namespace eastl
{
    /// EASTL_FIXED_LIST_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    /// In the case of fixed-size containers, the allocator name always refers
    /// to overflow allocations. 
    ///
    #ifndef EASTL_FIXED_LIST_DEFAULT_NAME
        #define EASTL_FIXED_LIST_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " fixed_list" // Unless the user overrides something, this is "EASTL fixed_list".
    #endif



    /// fixed_list
    ///
    /// fixed_list is a list which uses a single block of contiguous memory 
    /// for its nodes. The purpose of this is to reduce memory usage relative
    /// to a conventional memory allocation system (with block headers), to 
    /// increase allocation speed (often due to avoidance of mutex locks),
    /// to increase performance (due to better memory locality), and to decrease
    /// memory fragmentation due to the way that fixed block allocators work.
    ///
    /// The primary downside to a fixed_list is that the number of nodes it
    /// can contain is fixed upon its declaration. If you want a fixed_list
    /// that doesn't have this limitation, then you probably don't want a
    /// fixed_list. You can always create your own memory allocator that works
    /// the way you want.
    ///
    /// Template parameters:
    ///     T                      The type of object the list holds.
    ///     nodeCount              The max number of objects to contain.
    ///     bEnableOverflow        Whether or not we should use the overflow heap if our object pool is exhausted.
    ///     Allocator              Overflow allocator, which is only used if bEnableOverflow == true. Defaults to the global heap.
    ///
    template <typename T, size_t nodeCount, bool bEnableOverflow = true, typename Allocator = EASTLAllocatorType>
    class fixed_list : public list<T, fixed_node_allocator<sizeof(typename list<T>::node_type), 
                                   nodeCount, list<T>::kAlignment, list<T>::kAlignmentOffset, bEnableOverflow, Allocator> >
    {
    public:
        typedef fixed_list<T, nodeCount, bEnableOverflow, Allocator>                             this_type;
        typedef fixed_node_allocator<sizeof(typename list<T>::node_type), nodeCount, 
                     list<T>::kAlignment, list<T>::kAlignmentOffset, bEnableOverflow, Allocator> fixed_allocator_type;
        typedef list<T, fixed_allocator_type>                                                    base_type;
        typedef typename base_type::size_type                                                    size_type;
        typedef typename base_type::value_type                                                   value_type;
        typedef typename base_type::node_type                                                    node_type;
        typedef typename base_type::iterator                                                     iterator;

        enum
        {
            kMaxSize = nodeCount
        };

        using base_type::assign;
        using base_type::resize;

    protected:
        char mBuffer[fixed_allocator_type::kBufferSize]; // kBufferSize will take into account alignment requirements.

        using base_type::mAllocator;

    public:
        fixed_list();
        explicit fixed_list(size_type n);
        fixed_list(size_type n, const value_type& value);
        fixed_list(const this_type& x);

        template <typename InputIterator>
        fixed_list(InputIterator first, InputIterator last);

        this_type& operator=(const this_type& x);

        void      swap(this_type& x);
        void      reset();
        size_type max_size() const;         // Returns the max fixed size, which is the user-supplied nodeCount parameter.
        bool      has_overflowed() const;   // Returns true if the fixed space is fully allocated. Note that if overflow is enabled, the container size can be greater than nodeCount but full() could return true because the fixed space may have a recently freed slot.

        template<typename Compare>
        void sort(Compare compare);
        void sort();

        template <typename Compare>
        void merge(this_type& x, Compare compare);
        void merge(this_type& x);

        void splice(iterator position, this_type& x);
        void splice(iterator position, this_type& x, iterator i);
        void splice(iterator position, this_type& x, iterator first, iterator last);

        // Deprecated:
        bool      full() const { return has_overflowed(); }

    }; // fixed_list



    ///////////////////////////////////////////////////////////////////////
    // fixed_list
    ///////////////////////////////////////////////////////////////////////

    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline fixed_list<T, nodeCount, bEnableOverflow, Allocator>::fixed_list()
        : base_type(fixed_allocator_type(NULL))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_LIST_DEFAULT_NAME);
        #endif

        mAllocator.reset(mBuffer);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline fixed_list<T, nodeCount, bEnableOverflow, Allocator>::fixed_list(size_type n)
        : base_type(fixed_allocator_type(NULL))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_LIST_DEFAULT_NAME);
        #endif

        mAllocator.reset(mBuffer);
        resize(n);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline fixed_list<T, nodeCount, bEnableOverflow, Allocator>::fixed_list(size_type n, const value_type& value)
        : base_type(fixed_allocator_type(NULL))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_LIST_DEFAULT_NAME);
        #endif

        mAllocator.reset(mBuffer);
        resize(n, value);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline fixed_list<T, nodeCount, bEnableOverflow, Allocator>::fixed_list(const this_type& x)
        : base_type(fixed_allocator_type(NULL))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(x.mAllocator.get_name());
        #endif

        mAllocator.reset(mBuffer);
        assign(x.begin(), x.end());
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    template <typename InputIterator>
    fixed_list<T, nodeCount, bEnableOverflow, Allocator>::fixed_list(InputIterator first, InputIterator last)
        : base_type(fixed_allocator_type(NULL))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_LIST_DEFAULT_NAME);
        #endif

        mAllocator.reset(mBuffer);
        assign(first, last);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline typename fixed_list<T, nodeCount, bEnableOverflow, Allocator>::this_type&
    fixed_list<T, nodeCount, bEnableOverflow, Allocator>::operator=(const this_type& x)
    {
        if(this != &x)
        {
            base_type::clear();

            #if EASTL_ALLOCATOR_COPY_ENABLED
                mAllocator = x.mAllocator;
            #endif

            base_type::assign(x.begin(), x.end()); // It would probably be better to implement this like list::operator=.
        }
        return *this;
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline void fixed_list<T, nodeCount, bEnableOverflow, Allocator>::swap(this_type& x)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(*this, x);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline void fixed_list<T, nodeCount, bEnableOverflow, Allocator>::reset()
    {
        base_type::reset();
        base_type::get_allocator().reset(mBuffer);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline typename fixed_list<T, nodeCount, bEnableOverflow, Allocator>::size_type
    fixed_list<T, nodeCount, bEnableOverflow, Allocator>::max_size() const
    {
        return kMaxSize;
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline bool fixed_list<T, nodeCount, bEnableOverflow, Allocator>::has_overflowed() const
    {
        return !mAllocator.can_allocate();
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline void fixed_list<T, nodeCount, bEnableOverflow, Allocator>::sort()
    {
        eastl::insertion_sort(base_type::begin(), base_type::end());
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    template <typename Compare>
    inline void fixed_list<T, nodeCount, bEnableOverflow, Allocator>::sort(Compare compare)
    {
        eastl::insertion_sort(base_type::begin(), base_type::end(), compare);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    void fixed_list<T, nodeCount, bEnableOverflow, Allocator>::merge(this_type& /*x*/)
    {
        // To do.
    }

    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    template <typename Compare>
    void fixed_list<T, nodeCount, bEnableOverflow, Allocator>::merge(this_type& /*x*/, Compare /*compare*/)
    {
        // To do.
    }

    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    void fixed_list<T, nodeCount, bEnableOverflow, Allocator>::splice(iterator /*position*/, this_type& /*x*/)
    {
        // To do.
    }

    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    void fixed_list<T, nodeCount, bEnableOverflow, Allocator>::splice(iterator /*position*/, this_type& /*x*/, iterator /*i*/)
    {
        // To do.
    }

    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    void fixed_list<T, nodeCount, bEnableOverflow, Allocator>::splice(iterator /*position*/, this_type& /*x*/, iterator /*first*/, iterator /*last*/)
    {
        // To do.
    }



    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline void swap(fixed_list<T, nodeCount, bEnableOverflow, Allocator>& a, 
                     fixed_list<T, nodeCount, bEnableOverflow, Allocator>& b)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(a, b);
    }


} // namespace eastl


#endif // Header include guard












