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
// EASTL/fixed_hash_set.h
//
// Copyright (c) 2005, Electronic Arts. All rights reserved.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements a hash_set which uses a fixed size memory pool for 
// its buckets and nodes. 
//
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_FIXED_HASH_SET_H
#define EASTL_FIXED_HASH_SET_H


#include <EASTL/hash_set.h>
#include <EASTL/internal/fixed_pool.h>



namespace eastl
{
    /// EASTL_FIXED_HASH_SET_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    /// In the case of fixed-size containers, the allocator name always refers
    /// to overflow allocations. 
    ///
    #ifndef EASTL_FIXED_HASH_SET_DEFAULT_NAME
        #define EASTL_FIXED_HASH_SET_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " fixed_hash_set" // Unless the user overrides something, this is "EASTL fixed_hash_set".
    #endif

    #ifndef EASTL_FIXED_HASH_MULTISET_DEFAULT_NAME
        #define EASTL_FIXED_HASH_MULTISET_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " fixed_hash_multiset" // Unless the user overrides something, this is "EASTL fixed_hash_multiset".
    #endif



    /// fixed_hash_set
    ///
    /// Implements a hash_set with a fixed block of memory identified by the nodeCount and bucketCount
    /// template parameters. 
    ///
    /// Template parameters:
    ///     Value                  The type of object the hash_set holds.
    ///     nodeCount              The max number of objects to contain. This value must be >= 1.
    ///     bucketCount            The number of buckets to use. This value must be >= 2.
    ///     bEnableOverflow        Whether or not we should use the global heap if our object pool is exhausted.
    ///     Hash                   hash_set hash function. See hash_set.
    ///     Predicate              hash_set equality testing function. See hash_set.
    ///
    template <typename Value, size_t nodeCount, size_t bucketCount = nodeCount + 1, bool bEnableOverflow = true,
              typename Hash = eastl::hash<Value>, typename Predicate = eastl::equal_to<Value>, bool bCacheHashCode = false, typename Allocator = EASTLAllocatorType>
    class fixed_hash_set : public hash_set<Value,
                                           Hash,
                                           Predicate,
                                           fixed_hashtable_allocator<
                                                bucketCount + 1, 
                                                sizeof(typename hash_set<Value, Hash, Predicate, Allocator, bCacheHashCode>::node_type), 
                                                nodeCount, 
                                                hash_set<Value, Hash, Predicate, Allocator, bCacheHashCode>::kValueAlignment, 
                                                hash_set<Value, Hash, Predicate, Allocator, bCacheHashCode>::kValueAlignmentOffset, 
                                                bEnableOverflow,
                                                Allocator>, 
                                           bCacheHashCode>
    {
    public:
        typedef fixed_hash_set<Value, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, Allocator> this_type;
        typedef fixed_hashtable_allocator<bucketCount + 1, sizeof(typename hash_set<Value, Hash, Predicate, 
                        Allocator, bCacheHashCode>::node_type), nodeCount, hash_set<Value, Hash, Predicate, 
                        Allocator, bCacheHashCode>::kValueAlignment, hash_set<Value, Hash, Predicate, 
                        Allocator, bCacheHashCode>::kValueAlignmentOffset, bEnableOverflow, Allocator>              fixed_allocator_type;
        typedef hash_set<Value, Hash, Predicate, fixed_allocator_type, bCacheHashCode>                              base_type;
        typedef typename base_type::node_type                                                                       node_type;
        typedef typename base_type::size_type                                                                       size_type;

        enum
        {
            kMaxSize = nodeCount
        };

        using base_type::mAllocator;

    protected:
        node_type** mBucketBuffer[bucketCount + 1]; // '+1' because the hash table needs a null terminating bucket.
        char        mNodeBuffer[fixed_allocator_type::kBufferSize]; // kBufferSize will take into account alignment requirements.

    public:
        /// fixed_hash_set
        ///
        /// Construct an empty fixed_hash_set with a given set of parameters.
        ///
        explicit fixed_hash_set(const Hash& hashFunction = Hash(), 
                                const Predicate& predicate = Predicate())
            : base_type(prime_rehash_policy::GetPrevBucketCountOnly(bucketCount), 
                        hashFunction, predicate, fixed_allocator_type(NULL, mBucketBuffer))
        {
            EASTL_CT_ASSERT((nodeCount >= 1) && (bucketCount >= 2));
            base_type::set_max_load_factor(10000.f); // Set it so that we will never resize.

            #if EASTL_NAME_ENABLED
                mAllocator.set_name(EASTL_FIXED_HASH_SET_DEFAULT_NAME);
            #endif

            mAllocator.reset(mNodeBuffer);
        }


        /// fixed_hash_set
        ///
        /// Construct a fixed_hash_set from a source sequence and with a given set of parameters.
        ///
        template <typename InputIterator>
        fixed_hash_set(InputIterator first, InputIterator last,
                       const Hash& hashFunction = Hash(),
                       const Predicate& predicate = Predicate())
            : base_type(prime_rehash_policy::GetPrevBucketCountOnly(bucketCount), hashFunction, 
                        predicate, fixed_allocator_type(NULL, mBucketBuffer))
        {
            EASTL_CT_ASSERT((nodeCount >= 1) && (bucketCount >= 2));
            base_type::set_max_load_factor(10000.f); // Set it so that we will never resize.

            #if EASTL_NAME_ENABLED
                mAllocator.set_name(EASTL_FIXED_HASH_SET_DEFAULT_NAME);
            #endif

            mAllocator.reset(mNodeBuffer);
            base_type::insert(first, last);
        }


        /// fixed_hash_set
        ///
        /// Copy constructor
        ///
        fixed_hash_set(const this_type& x)
            : base_type(prime_rehash_policy::GetPrevBucketCountOnly(bucketCount), x.hash_function(),
                        x.equal_function(), fixed_allocator_type(NULL, mBucketBuffer))
        {
            EASTL_CT_ASSERT((nodeCount >= 1) && (bucketCount >= 2));
            base_type::set_max_load_factor(10000.f); // Set it so that we will never resize.

            #if EASTL_NAME_ENABLED
                mAllocator.set_name(x.mAllocator.get_name());
            #endif

            mAllocator.reset(mNodeBuffer);
            base_type::insert(x.begin(), x.end());
        }


        /// operator=
        ///
        /// We provide an override so that assignment is done correctly.
        ///
        this_type& operator=(const this_type& x)
        {
            if(this != &x)
            {
                base_type::clear();
                base_type::insert(x.begin(), x.end());
            }
            return *this;
        }


        void swap(this_type& x)
        {
            // We must do a brute-force swap, because fixed containers cannot share memory allocations.
            // Note that we create a temp value on the stack. This approach may fail if the size of the 
            // container is too large. We have a rule against allocating memory from the heap, and so 
            // if the user wants to swap two large objects of this class, the user will currently need 
            // to implement it manually. To consider: add code to allocate a temporary buffer if the 
            // size of the container is too large for the stack.
            EASTL_ASSERT(sizeof(x) < EASTL_MAX_STACK_USAGE); // It is dangerous to try to create objects that are too big for the stack.

            const this_type temp(*this); // Can't call eastl::swap because that would
            *this = x;                   // itself call this member swap function.
            x     = temp;
        }


        void reset()
        {
            base_type::reset();
            base_type::get_allocator().reset(mNodeBuffer);
        }


        size_type max_size() const
        {
            return kMaxSize;
        }

    }; // fixed_hash_set


    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template <typename Value, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode>
    inline void swap(fixed_hash_set<Value, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode>& a, 
                     fixed_hash_set<Value, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode>& b)
    {
        a.swap(b);
    }






    /// fixed_hash_multiset
    ///
    /// Implements a hash_multiset with a fixed block of memory identified by the nodeCount and bucketCount
    /// template parameters. 
    ///
    ///     Value                  The type of object the hash_set holds.
    ///     nodeCount              The max number of objects to contain. This value must be >= 1.
    ///     bucketCount            The number of buckets to use. This value must be >= 2.
    ///     bEnableOverflow        Whether or not we should use the global heap if our object pool is exhausted.
    ///     Hash                   hash_set hash function. See hash_set.
    ///     Predicate              hash_set equality testing function. See hash_set.
    ///
    template <typename Value, size_t nodeCount, size_t bucketCount = nodeCount + 1, bool bEnableOverflow = true,
              typename Hash = eastl::hash<Value>, typename Predicate = eastl::equal_to<Value>, bool bCacheHashCode = false, typename Allocator = EASTLAllocatorType>
    class fixed_hash_multiset : public hash_multiset<Value,
                                                     Hash,
                                                     Predicate,
                                                     fixed_hashtable_allocator<
                                                        bucketCount + 1, 
                                                        sizeof(typename hash_multiset<Value, Hash, Predicate, Allocator, bCacheHashCode>::node_type),
                                                        nodeCount,
                                                        hash_multiset<Value, Hash, Predicate, Allocator, bCacheHashCode>::kValueAlignment, 
                                                        hash_multiset<Value, Hash, Predicate, Allocator, bCacheHashCode>::kValueAlignmentOffset, 
                                                        bEnableOverflow,
                                                        Allocator>,
                                                     bCacheHashCode>
    {
    public:
        typedef fixed_hash_multiset<Value, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, Allocator> this_type;
        typedef fixed_hashtable_allocator<bucketCount + 1, sizeof(typename hash_multiset<Value, Hash, Predicate, 
                    Allocator, bCacheHashCode>::node_type), nodeCount, hash_multiset<Value, Hash, Predicate, 
                    Allocator, bCacheHashCode>::kValueAlignment, hash_multiset<Value, Hash, Predicate, 
                    Allocator, bCacheHashCode>::kValueAlignmentOffset, bEnableOverflow, Allocator>                          fixed_allocator_type;
        typedef hash_multiset<Value, Hash, Predicate, fixed_allocator_type, bCacheHashCode>                                 base_type;
        typedef typename base_type::node_type                                                                               node_type;
        typedef typename base_type::size_type                                                                               size_type;

        enum
        {
            kMaxSize = nodeCount
        };

        using base_type::mAllocator;

    protected:
        node_type** mBucketBuffer[bucketCount + 1]; // '+1' because the hash table needs a null terminating bucket.
        char        mNodeBuffer[fixed_allocator_type::kBufferSize]; // kBufferSize will take into account alignment requirements.

    public:
        /// fixed_hash_multiset
        ///
        /// Construct an empty fixed_hash_multiset with a given set of parameters.
        ///
        explicit fixed_hash_multiset(const Hash& hashFunction = Hash(), 
                                     const Predicate& predicate = Predicate())
            : base_type(prime_rehash_policy::GetPrevBucketCountOnly(bucketCount), hashFunction, 
                        predicate, fixed_allocator_type(NULL, mBucketBuffer))
        {
            EASTL_CT_ASSERT((nodeCount >= 1) && (bucketCount >= 2));
            base_type::set_max_load_factor(10000.f); // Set it so that we will never resize.

            #if EASTL_NAME_ENABLED
                mAllocator.set_name(EASTL_FIXED_HASH_MULTISET_DEFAULT_NAME);
            #endif

            mAllocator.reset(mNodeBuffer);
        }


        /// fixed_hash_multiset
        ///
        /// Construct a fixed_hash_multiset from a source sequence and with a given set of parameters.
        ///
        template <typename InputIterator>
        fixed_hash_multiset(InputIterator first, InputIterator last, 
                            const Hash& hashFunction = Hash(), 
                            const Predicate& predicate = Predicate())
            : base_type(prime_rehash_policy::GetPrevBucketCountOnly(bucketCount), hashFunction, 
                        predicate, fixed_allocator_type(NULL, mBucketBuffer))
        {
            EASTL_CT_ASSERT((nodeCount >= 1) && (bucketCount >= 2));
            base_type::set_max_load_factor(10000.f); // Set it so that we will never resize.

            #if EASTL_NAME_ENABLED
                mAllocator.set_name(EASTL_FIXED_HASH_MULTISET_DEFAULT_NAME);
            #endif

            mAllocator.reset(mNodeBuffer);
            base_type::insert(first, last);
        }


        /// fixed_hash_multiset
        ///
        /// Copy constructor
        ///
        fixed_hash_multiset(const this_type& x)
            : base_type(prime_rehash_policy::GetPrevBucketCountOnly(bucketCount), x.hash_function(), 
                        x.equal_function(), fixed_allocator_type(NULL, mBucketBuffer))
        {
            EASTL_CT_ASSERT((nodeCount >= 1) && (bucketCount >= 2));
            base_type::set_max_load_factor(10000.f); // Set it so that we will never resize.

            #if EASTL_NAME_ENABLED
                mAllocator.set_name(x.mAllocator.get_name());
            #endif

            mAllocator.reset(mNodeBuffer);
            base_type::insert(x.begin(), x.end());
        }


        /// operator=
        ///
        /// We provide an override so that assignment is done correctly.
        ///
        this_type& operator=(const this_type& x)
        {
            if(this != &x)
            {
                base_type::clear();
                base_type::insert(x.begin(), x.end());
            }
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
            base_type::get_allocator().reset(mNodeBuffer);
        }


        size_type max_size() const
        {
            return kMaxSize;
        }

    }; // fixed_hash_multiset


    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template <typename Value, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode>
    inline void swap(fixed_hash_multiset<Value, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode>& a, 
                     fixed_hash_multiset<Value, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode>& b)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(a, b);
    }


} // namespace eastl


#endif // Header include guard












