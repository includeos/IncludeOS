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
// EASTL/fixed_map.h
//
// Copyright (c) 2005, Electronic Arts. All rights reserved.
// Written by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements a map and multimap which use a fixed size memory 
// pool for their nodes. 
//
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_FIXED_MAP_H
#define EASTL_FIXED_MAP_H


#include <EASTL/map.h>
#include <EASTL/fixed_set.h> // Included because fixed_rbtree_base resides here.


namespace eastl
{
    /// EASTL_FIXED_MAP_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    /// In the case of fixed-size containers, the allocator name always refers
    /// to overflow allocations. 
    ///
    #ifndef EASTL_FIXED_MAP_DEFAULT_NAME
        #define EASTL_FIXED_MAP_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " fixed_map" // Unless the user overrides something, this is "EASTL fixed_map".
    #endif

    #ifndef EASTL_FIXED_MULTIMAP_DEFAULT_NAME
        #define EASTL_FIXED_MULTIMAP_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " fixed_multimap" // Unless the user overrides something, this is "EASTL fixed_multimap".
    #endif



    /// fixed_map
    ///
    /// Implements a map with a fixed block of memory identified by the 
    /// nodeCount template parameter. 
    ///
    ///     Key                    The key object (key in the key/value pair).
    ///     T                      The mapped object (value in the key/value pair).
    ///     nodeCount              The max number of objects to contain.
    ///     bEnableOverflow        Whether or not we should use the global heap if our object pool is exhausted.
    ///     Compare                Compare function/object for set ordering.
    ///     Allocator              Overflow allocator, which is only used if bEnableOverflow == true. Defaults to the global heap.
    ///
    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow = true, typename Compare = eastl::less<Key>, typename Allocator = EASTLAllocatorType>
    class fixed_map : public map<Key, T, Compare, fixed_node_allocator<sizeof(typename map<Key, T>::node_type), 
                                 nodeCount, map<Key, T>::kValueAlignment, map<Key, T>::kValueAlignmentOffset, bEnableOverflow, Allocator> >
    {
    public:
        typedef fixed_map<Key, T, nodeCount, bEnableOverflow, Compare, Allocator>                                  this_type;
        typedef fixed_node_allocator<sizeof(typename map<Key, T>::node_type), nodeCount, 
                     map<Key, T>::kValueAlignment, map<Key, T>::kValueAlignmentOffset, bEnableOverflow, Allocator> fixed_allocator_type;
        typedef map<Key, T, Compare, fixed_allocator_type>                                                         base_type;
        typedef typename base_type::value_type                                                                     value_type;
        typedef typename base_type::node_type                                                                      node_type;
        typedef typename base_type::size_type                                                                      size_type;

        enum
        {
            kMaxSize = nodeCount
        };

        using base_type::insert;

    protected:
        char mBuffer[fixed_allocator_type::kBufferSize]; // kBufferSize will take into account alignment requirements.

        using base_type::mAllocator;

    public:
        /// fixed_map
        ///
        fixed_map()
            : base_type(fixed_allocator_type(NULL))
        {
            #if EASTL_NAME_ENABLED
                mAllocator.set_name(EASTL_FIXED_MAP_DEFAULT_NAME);
            #endif

            mAllocator.reset(mBuffer);
        }


        /// fixed_map
        ///
        explicit fixed_map(const Compare& compare)
            : base_type(compare, fixed_allocator_type(NULL))
        {
            #if EASTL_NAME_ENABLED
                mAllocator.set_name(EASTL_FIXED_MAP_DEFAULT_NAME);
            #endif

            mAllocator.reset(mBuffer);
        }


        /// fixed_map
        ///
        fixed_map(const this_type& x)
            : base_type(x.mCompare, fixed_allocator_type(NULL))
        {
            #if EASTL_NAME_ENABLED
                mAllocator.set_name(x.mAllocator.get_name());
            #endif

            mAllocator.reset(mBuffer);
            base_type::operator=(x);
        }


        /// fixed_map
        ///
        template <typename InputIterator>
        fixed_map(InputIterator first, InputIterator last)
            : base_type(fixed_allocator_type(NULL))
        {
            #if EASTL_NAME_ENABLED
                mAllocator.set_name(EASTL_FIXED_MAP_DEFAULT_NAME);
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

    }; // fixed_map


    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename Allocator>
    inline void swap(fixed_map<Key, T, nodeCount, bEnableOverflow, Compare, Allocator>& a, 
                     fixed_map<Key, T, nodeCount, bEnableOverflow, Compare, Allocator>& b)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(a, b);
    }





    /// fixed_multimap
    ///
    /// Implements a multimap with a fixed block of memory identified by the 
    /// nodeCount template parameter. 
    ///
    ///     Key                    The key object (key in the key/value pair).
    ///     T                      The mapped object (value in the key/value pair).
    ///     nodeCount              The max number of objects to contain.
    ///     bEnableOverflow        Whether or not we should use the global heap if our object pool is exhausted.
    ///     Compare                Compare function/object for set ordering.
    ///     Allocator              Overflow allocator, which is only used if bEnableOverflow == true. Defaults to the global heap.
    ///
    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow = true, typename Compare = eastl::less<Key>, typename Allocator = EASTLAllocatorType>
    class fixed_multimap : public multimap<Key, T, Compare, fixed_node_allocator<sizeof(typename multimap<Key, T>::node_type), 
                                           nodeCount, multimap<Key, T>::kValueAlignment, multimap<Key, T>::kValueAlignmentOffset, bEnableOverflow, Allocator> >
    {
    public:
        typedef fixed_multimap<Key, T, nodeCount, bEnableOverflow, Compare, Allocator>                                       this_type;
        typedef fixed_node_allocator<sizeof(typename multimap<Key, T>::node_type), nodeCount, 
                     multimap<Key, T>::kValueAlignment, multimap<Key, T>::kValueAlignmentOffset, bEnableOverflow, Allocator> fixed_allocator_type;
        typedef multimap<Key, T, Compare, fixed_allocator_type>                                                              base_type;
        typedef typename base_type::value_type                                                                               value_type;
        typedef typename base_type::node_type                                                                                node_type;
        typedef typename base_type::size_type                                                                                size_type;

        enum
        {
            kMaxSize = nodeCount
        };

        using base_type::insert;

    protected:
        char mBuffer[fixed_allocator_type::kBufferSize]; // kBufferSize will take into account alignment requirements.

        using base_type::mAllocator;

    public:
        /// fixed_multimap
        ///
        fixed_multimap()
            : base_type(fixed_allocator_type(NULL))
        {
            #if EASTL_NAME_ENABLED
                mAllocator.set_name(EASTL_FIXED_MULTIMAP_DEFAULT_NAME);
            #endif

            mAllocator.reset(mBuffer);
        }


        /// fixed_multimap
        ///
        explicit fixed_multimap(const Compare& compare)
            : base_type(compare, fixed_allocator_type(mBuffer))
        {
            #if EASTL_NAME_ENABLED
                mAllocator.set_name(EASTL_FIXED_MULTIMAP_DEFAULT_NAME);
            #endif

            mAllocator.reset(mBuffer);
        }


        /// fixed_multimap
        ///
        fixed_multimap(const this_type& x)
            : base_type(x.mCompare, fixed_allocator_type(NULL))
        {
            #if EASTL_NAME_ENABLED
                mAllocator.set_name(x.mAllocator.get_name());
            #endif

            mAllocator.reset(mBuffer);
            base_type::operator=(x);
        }


        /// fixed_multimap
        ///
        template <typename InputIterator>
        fixed_multimap(InputIterator first, InputIterator last)
            : base_type(fixed_allocator_type(NULL))
        {
            #if EASTL_NAME_ENABLED
                mAllocator.set_name(EASTL_FIXED_MULTIMAP_DEFAULT_NAME);
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

    }; // fixed_multimap


    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename Allocator>
    inline void swap(fixed_multimap<Key, T, nodeCount, bEnableOverflow, Compare, Allocator>& a, 
                     fixed_multimap<Key, T, nodeCount, bEnableOverflow, Compare, Allocator>& b)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(a, b);
    }


} // namespace eastl


#endif // Header include guard









