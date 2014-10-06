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
// EASTL/fixed_vector.h
//
// Copyright (c) 2005, Electronic Arts. All rights reserved.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements a vector which uses a fixed size memory pool. 
// The bEnableOverflow template parameter allows the container to resort to
// heap allocations if the memory pool is exhausted.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_FIXED_VECTOR_H
#define EASTL_FIXED_VECTOR_H


#include <EASTL/vector.h>
#include <EASTL/internal/fixed_pool.h>


namespace eastl
{
    /// EASTL_FIXED_VECTOR_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    /// In the case of fixed-size containers, the allocator name always refers
    /// to overflow allocations. 
    ///
    #ifndef EASTL_FIXED_VECTOR_DEFAULT_NAME
        #define EASTL_FIXED_VECTOR_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " fixed_vector" // Unless the user overrides something, this is "EASTL fixed_vector".
    #endif


    /// fixed_vector
    ///
    /// A fixed_vector with bEnableOverflow == true is identical to a regular 
    /// vector in terms of its behavior. All the expectations of regular vector
    /// apply to it and no additional expectations come from it. When bEnableOverflow
    /// is false, fixed_vector behaves like regular vector with the exception that 
    /// its capacity can never increase. All operations you do on such a fixed_vector
    /// which require a capacity increase will result in undefined behavior or an 
    /// C++ allocation exception, depending on the configuration of EASTL.
    ///
    /// Template parameters:
    ///     T                      The type of object the vector holds.
    ///     nodeCount              The max number of objects to contain.
    ///     bEnableOverflow        Whether or not we should use the overflow heap if our object pool is exhausted.
    ///     Allocator              Overflow allocator, which is only used if bEnableOverflow == true. Defaults to the global heap.
    ///
    /// Note: The nodeCount value must be at least 1.
    ///
    /// Example usage:
    ///    fixed_vector<Widget, 128, true> fixedVector);
    ///
    ///    fixedVector.push_back(Widget());
    ///    fixedVector.resize(200);
    ///    fixedVector.clear();
    ///
    template <typename T, size_t nodeCount, bool bEnableOverflow = true, typename Allocator = EASTLAllocatorType>
    class fixed_vector : public vector<T, fixed_vector_allocator<sizeof(T), nodeCount, vector<T>::kAlignment, vector<T>::kAlignmentOffset, bEnableOverflow, Allocator> >
    {
    public:
        typedef fixed_vector_allocator<sizeof(T), nodeCount, vector<T>::kAlignment, 
                            vector<T>::kAlignmentOffset, bEnableOverflow, Allocator>    fixed_allocator_type;
        typedef vector<T, fixed_allocator_type>                                         base_type;
        typedef fixed_vector<T, nodeCount, bEnableOverflow, Allocator>                  this_type;
        typedef typename base_type::size_type                                           size_type;
        typedef typename base_type::value_type                                          value_type;
        typedef aligned_buffer<nodeCount * sizeof(T), vector<T>::kAlignment>            aligned_buffer_type;

        enum
        {
            kMaxSize = nodeCount
        };

        using base_type::mAllocator;
        using base_type::mpBegin;
        using base_type::mpEnd;
        using base_type::mpCapacity;
        using base_type::resize;
        using base_type::clear;
        using base_type::size;
        using base_type::assign;

    protected:
        aligned_buffer_type mBuffer;

    public:
        fixed_vector();
        explicit fixed_vector(size_type n);
        fixed_vector(size_type n, const value_type& value);
        fixed_vector(const this_type& x);

        template <typename InputIterator>
        fixed_vector(InputIterator first, InputIterator last);

        this_type& operator=(const this_type& x);

        void swap(this_type& x);

        void      set_capacity(size_type n);
        void      reset();
        size_type max_size() const;       // Returns the max fixed size, which is the user-supplied nodeCount parameter.
        bool      has_overflowed() const; // Returns true if the fixed space is fully allocated. Note that if overflow is enabled, the container size can be greater than nodeCount but full() could return true because the fixed space may have a recently freed slot.

        void*     push_back_uninitialized();

        // Deprecated:
        bool      full() const { return has_overflowed(); }

    protected:
        void*     DoPushBackUninitialized(true_type);
        void*     DoPushBackUninitialized(false_type);

    }; // fixed_vector




    ///////////////////////////////////////////////////////////////////////
    // fixed_vector
    ///////////////////////////////////////////////////////////////////////

    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline fixed_vector<T, nodeCount, bEnableOverflow, Allocator>::fixed_vector()
        : base_type(fixed_allocator_type(mBuffer.buffer))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_VECTOR_DEFAULT_NAME);
        #endif

        mpBegin = mpEnd = (value_type*)&mBuffer.buffer[0];
        mpCapacity = mpBegin + nodeCount;
    }

    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline fixed_vector<T, nodeCount, bEnableOverflow, Allocator>::fixed_vector(size_type n)
        : base_type(fixed_allocator_type(mBuffer.buffer))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_VECTOR_DEFAULT_NAME);
        #endif

        mpBegin = mpEnd = (value_type*)&mBuffer.buffer[0];
        mpCapacity = mpBegin + nodeCount;
        resize(n);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline fixed_vector<T, nodeCount, bEnableOverflow, Allocator>::fixed_vector(size_type n, const value_type& value)
        : base_type(fixed_allocator_type(mBuffer.buffer))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_VECTOR_DEFAULT_NAME);
        #endif

        mpBegin = mpEnd = (value_type*)&mBuffer.buffer[0];
        mpCapacity = mpBegin + nodeCount;
        resize(n, value);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline fixed_vector<T, nodeCount, bEnableOverflow, Allocator>::fixed_vector(const this_type& x)
        : base_type(fixed_allocator_type(mBuffer.buffer))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(x.mAllocator.get_name());
        #endif

        mpBegin = mpEnd = (value_type*)&mBuffer.buffer[0];
        mpCapacity = mpBegin + nodeCount;
        assign(x.begin(), x.end());
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    template <typename InputIterator>
    fixed_vector<T, nodeCount, bEnableOverflow, Allocator>::fixed_vector(InputIterator first, InputIterator last)
        : base_type(fixed_allocator_type(mBuffer.buffer))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_VECTOR_DEFAULT_NAME);
        #endif

        mpBegin = mpEnd = (value_type*)&mBuffer.buffer[0];
        mpCapacity = mpBegin + nodeCount;
        //assign(first, last); // Metrowerks gets confused by this.
        base_type::DoAssign(first, last, is_integral<InputIterator>());
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline typename fixed_vector<T, nodeCount, bEnableOverflow, Allocator>::this_type& 
    fixed_vector<T, nodeCount, bEnableOverflow, Allocator>::operator=(const this_type& x)
    {
        if(this != &x)
        {
            clear();
            assign(x.begin(), x.end());
        }
        return *this;
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline void fixed_vector<T, nodeCount, bEnableOverflow, Allocator>::swap(this_type& x)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(*this, x);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline void fixed_vector<T, nodeCount, bEnableOverflow, Allocator>::set_capacity(size_type n)
    {
        // We act consistently with vector::set_capacity and reduce our 
        // size if the new capacity is smaller than our size.
        if(n < size())
            resize(n);
        // To consider: If bEnableOverflow is true, then perhaps we should
        // switch to the overflow allocator and set the capacity.
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline void fixed_vector<T, nodeCount, bEnableOverflow, Allocator>::reset()
    {
        mpBegin = mpEnd = (value_type*)&mBuffer.buffer[0];
        mpCapacity = mpBegin + nodeCount;
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline typename fixed_vector<T, nodeCount, bEnableOverflow, Allocator>::size_type
    fixed_vector<T, nodeCount, bEnableOverflow, Allocator>::max_size() const
    {
        return kMaxSize;
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline bool fixed_vector<T, nodeCount, bEnableOverflow, Allocator>::has_overflowed() const
    {
        // If size >= capacity, then we are definitely full. 
        // Also, if our size is smaller but we've switched away from mBuffer due to a previous overflow, then we are considered full.
        return ((size_t)(mpEnd - mpBegin) >= kMaxSize) || ((void*)mpBegin != (void*)mBuffer.buffer);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline void* fixed_vector<T, nodeCount, bEnableOverflow, Allocator>::push_back_uninitialized()
    {
        return DoPushBackUninitialized(typename type_select<bEnableOverflow, true_type, false_type>::type());
    }

    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline void* fixed_vector<T, nodeCount, bEnableOverflow, Allocator>::DoPushBackUninitialized(true_type)
    {
        return base_type::push_back_uninitialized();
    }

    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline void* fixed_vector<T, nodeCount, bEnableOverflow, Allocator>::DoPushBackUninitialized(false_type)
    {
        return mpEnd++;
    }


    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    // operator ==, !=, <, >, <=, >= come from the vector implementations.

    template <typename T, size_t nodeCount, bool bEnableOverflow, typename Allocator>
    inline void swap(fixed_vector<T, nodeCount, bEnableOverflow, Allocator>& a, 
                     fixed_vector<T, nodeCount, bEnableOverflow, Allocator>& b)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(a, b);
    }



} // namespace eastl



#endif // Header include guard












