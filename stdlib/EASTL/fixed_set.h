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
// EASTL/fixed_set.h
//
// Copyright (c) 2005, Electronic Arts. All rights reserved.
// Written by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements a set and multiset which use a fixed size memory 
// pool for their nodes. 
//
///////////////////////////////////////////////////////////////////////////////



#ifndef EASTL_FIXED_SET_H
#define EASTL_FIXED_SET_H


#include <EASTL/set.h>
#include <EASTL/internal/fixed_pool.h>


namespace eastl
{
    /// EASTL_FIXED_SET_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    /// In the case of fixed-size containers, the allocator name always refers
    /// to overflow allocations. 
    ///
    #ifndef EASTL_FIXED_SET_DEFAULT_NAME
        #define EASTL_FIXED_SET_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " fixed_set" // Unless the user overrides something, this is "EASTL fixed_set".
    #endif

    #ifndef EASTL_FIXED_MULTISET_DEFAULT_NAME
        #define EASTL_FIXED_MULTISET_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " fixed_multiset" // Unless the user overrides something, this is "EASTL fixed_multiset".
    #endif



    /// fixed_set
    ///
    /// Implements a set with a fixed block of memory identified by the 
    /// nodeCount template parameter. 
    ///
    /// Template parameters:
    ///     Key                    The type of object the set holds (a.k.a. value).
    ///     nodeCount              The max number of objects to contain.
    ///     bEnableOverflow        Whether or not we should use the global heap if our object pool is exhausted.
    ///     Compare                Compare function/object for set ordering.
    ///     Allocator              Overflow allocator, which is only used if bEnableOverflow == true. Defaults to the global heap.
    ///
    template <typename Key, size_t nodeCount, bool bEnableOverflow = true, typename Compare = eastl::less<Key>, typename Allocator = EASTLAllocatorType>
    class fixed_set : public set<Key, Compare, fixed_node_allocator<sizeof(typename set<Key>::node_type), 
                                 nodeCount, set<Key>::kValueAlignment, set<Key>::kValueAlignmentOffset, bEnableOverflow, Allocator> >
    {
    public:
        typedef fixed_set<Key, nodeCount, bEnableOverflow, Compare, Allocator>                              this_type;
        typedef fixed_node_allocator<sizeof(typename set<Key>::node_type), nodeCount, 
                    set<Key>::kValueAlignment, set<Key>::kValueAlignmentOffset, bEnableOverflow, Allocator> fixed_allocator_type;
        typedef set<Key, Compare, fixed_allocator_type>                                                     base_type;
        typedef typename base_type::node_type                                                               node_type;
        typedef typename base_type::size_type                                                               size_type;

        enum
        {
            kMaxSize = nodeCount
        };

        using base_type::insert;

    protected:
        char mBuffer[fixed_allocator_type::kBufferSize]; // kBufferSize will take into account alignment requirements.

        using base_type::mAllocator;

    public:
        /// fixed_set
        ///
        fixed_set()
            : base_type(fixed_allocator_type(NULL))
        {
            #if EASTL_NAME_ENABLED
                mAllocator.set_name(EASTL_FIXED_SET_DEFAULT_NAME);
            #endif

            mAllocator.reset(mBuffer);
        }


        /// fixed_set
        ///
        explicit fixed_set(const Compare& compare)
            : base_type(compare, fixed_allocator_type(NULL))
        {
            #if EASTL_NAME_ENABLED
                mAllocator.set_name(EASTL_FIXED_SET_DEFAULT_NAME);
            #endif

            mAllocator.reset(mBuffer);
        }


        /// fixed_set
        ///
        fixed_set(const this_type& x)
            : base_type(x.mCompare, fixed_allocator_type(NULL))
        {
            #if EASTL_NAME_ENABLED
                mAllocator.set_name(x.mAllocator.get_name());
            #endif

            mAllocator.reset(mBuffer);
            base_type::operator=(x);
        }


        /// fixed_set
        ///
        template <typename InputIterator>
        fixed_set(InputIterator first, InputIterator last)
            : base_type(fixed_allocator_type(NULL))
        {
            #if EASTL_NAME_ENABLED
                mAllocator.set_name(EASTL_FIXED_SET_DEFAULT_NAME);
            #endif

            mAllocator.reset(mBuffer);
            insert(first, last);
        }


        /// operator=
        ///
        this_type& operator=(const this_type& x)
        {
            base_type::operator=(x);
            return *this;
        }


        void swap(this_type& x)
        {
            // Fixed containers use a special swap that can deal with excessively large buffers.
            eastl::fixed_swap(*this, x);
        }


        void reset()
        {
            base_type::reset();
            base_type::get_allocator().reset(mBuffer);
        }


        size_type max_size() const
        {
            return kMaxSize;
        }

    }; // fixed_set


    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template <typename Key, size_t nodeCount, bool bEnableOverflow, typename Compare, typename Allocator>
    inline void swap(fixed_set<Key, nodeCount, bEnableOverflow, Compare, Allocator>& a, 
                     fixed_set<Key, nodeCount, bEnableOverflow, Compare, Allocator>& b)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(a, b);
    }








    /// fixed_multiset
    ///
    /// Implements a multiset with a fixed block of memory identified by the 
    /// nodeCount template parameter. 
    ///
    ///     Key                    The type of object the set holds (a.k.a. value).
    ///     nodeCount              The max number of objects to contain.
    ///     bEnableOverflow        Whether or not we should use the global heap if our object pool is exhausted.
    ///     Compare                Compare function/object for set ordering.
    ///     Allocator              Overflow allocator, which is only used if bEnableOverflow == true. Defaults to the global heap.
    ///
    template <typename Key, size_t nodeCount, bool bEnableOverflow = true, typename Compare = eastl::less<Key>, typename Allocator = EASTLAllocatorType>
    class fixed_multiset : public multiset<Key, Compare, fixed_node_allocator<sizeof(typename multiset<Key>::node_type), 
                                           nodeCount, multiset<Key>::kValueAlignment, multiset<Key>::kValueAlignmentOffset, bEnableOverflow, Allocator> >
    {
    public:
        typedef fixed_multiset<Key, nodeCount, bEnableOverflow, Compare, Allocator>                                     this_type;
        typedef fixed_node_allocator<sizeof(typename multiset<Key>::node_type), nodeCount, 
                     multiset<Key>::kValueAlignment, multiset<Key>::kValueAlignmentOffset, bEnableOverflow, Allocator>  fixed_allocator_type;
        typedef multiset<Key, Compare, fixed_allocator_type>                                                            base_type;
        typedef typename base_type::node_type                                                                           node_type;
        typedef typename base_type::size_type                                                                           size_type;

        enum
        {
            kMaxSize = nodeCount
        };

        using base_type::insert;

    protected:
        char mBuffer[fixed_allocator_type::kBufferSize]; // kBufferSize will take into account alignment requirements.

        using base_type::mAllocator;

    public:
        /// fixed_multiset
        ///
        fixed_multiset()
            : base_type(fixed_allocator_type(NULL))
        {
            #if EASTL_NAME_ENABLED
                mAllocator.set_name(EASTL_FIXED_MULTISET_DEFAULT_NAME);
            #endif

            mAllocator.reset(mBuffer);
        }


        /// fixed_multiset
        ///
        explicit fixed_multiset(const Compare& compare)
            : base_type(compare, fixed_allocator_type(NULL))
        {
            #if EASTL_NAME_ENABLED
                mAllocator.set_name(EASTL_FIXED_MULTISET_DEFAULT_NAME);
            #endif

            mAllocator.reset(mBuffer);
        }


        /// fixed_multiset
        ///
        fixed_multiset(const this_type& x)
            : base_type(x.mCompare, fixed_allocator_type(NULL))
        {
            #if EASTL_NAME_ENABLED
                mAllocator.set_name(x.mAllocator.get_name());
            #endif

            mAllocator.reset(mBuffer);
            base_type::operator=(x);
        }


        /// fixed_multiset
        ///
        template <typename InputIterator>
        fixed_multiset(InputIterator first, InputIterator last)
            : base_type(fixed_allocator_type(NULL))
        {
            #if EASTL_NAME_ENABLED
                mAllocator.set_name(EASTL_FIXED_MULTISET_DEFAULT_NAME);
            #endif

            mAllocator.reset(mBuffer);
            insert(first, last);
        }


        /// operator=
        ///
        this_type& operator=(const this_type& x)
        {
            base_type::operator=(x);
            return *this;
        }


        void swap(this_type& x)
        {
            // Fixed containers use a special swap that can deal with excessively large buffers.
            eastl::fixed_swap(*this, x);
        }


        void reset()
        {
            base_type::reset();
            base_type::get_allocator().reset(mBuffer);
        }


        size_type max_size() const
        {
            return kMaxSize;
        }

    }; // fixed_multiset


    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template <typename Key, size_t nodeCount, bool bEnableOverflow, typename Compare, typename Allocator>
    inline void swap(fixed_multiset<Key, nodeCount, bEnableOverflow, Compare, Allocator>& a, 
                     fixed_multiset<Key, nodeCount, bEnableOverflow, Compare, Allocator>& b)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(a, b);
    }



} // namespace eastl


#endif // Header include guard









