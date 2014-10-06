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
// EASTL/hash_set.h
//
// Copyright (c) 2005, Electronic Arts. All rights reserved.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file is based on the TR1 (technical report 1) reference implementation
// of the unordered_set/unordered_map C++ classes as of about 4/2005. Most likely
// many or all C++ library vendors' implementations of this classes will be 
// based off of the reference version and so will look pretty similar to this
// file as well as other vendors' versions. 
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_HASH_SET_H
#define EASTL_HASH_SET_H


#include <EASTL/internal/config.h>
#include <EASTL/internal/hashtable.h>
#include <EASTL/functional.h>
#include <EASTL/utility.h>



namespace eastl
{

    /// EASTL_HASH_SET_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    ///
    #ifndef EASTL_HASH_SET_DEFAULT_NAME
        #define EASTL_HASH_SET_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " hash_set" // Unless the user overrides something, this is "EASTL hash_set".
    #endif


    /// EASTL_HASH_MULTISET_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    ///
    #ifndef EASTL_HASH_MULTISET_DEFAULT_NAME
        #define EASTL_HASH_MULTISET_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " hash_multiset" // Unless the user overrides something, this is "EASTL hash_multiset".
    #endif


    /// EASTL_HASH_SET_DEFAULT_ALLOCATOR
    ///
    #ifndef EASTL_HASH_SET_DEFAULT_ALLOCATOR
        #define EASTL_HASH_SET_DEFAULT_ALLOCATOR allocator_type(EASTL_HASH_SET_DEFAULT_NAME)
    #endif

    /// EASTL_HASH_MULTISET_DEFAULT_ALLOCATOR
    ///
    #ifndef EASTL_HASH_MULTISET_DEFAULT_ALLOCATOR
        #define EASTL_HASH_MULTISET_DEFAULT_ALLOCATOR allocator_type(EASTL_HASH_MULTISET_DEFAULT_NAME)
    #endif



    /// hash_set
    ///
    /// Implements a hash_set, which is a hashed unique-item container.
    /// Lookups are O(1) (that is, they are fast) but the container is 
    /// not sorted.
    ///
    /// set_max_load_factor
    /// If you want to make a hashtable never increase its bucket usage,
    /// call set_max_load_factor with a very high value such as 100000.f.
    ///
    /// bCacheHashCode
    /// We provide the boolean bCacheHashCode template parameter in order 
    /// to allow the storing of the hash code of the key within the map. 
    /// When this option is disabled, the rehashing of the table will 
    /// call the hash function on the key. Setting bCacheHashCode to true 
    /// is useful for cases whereby the calculation of the hash value for
    /// a contained object is very expensive.
    ///
    /// find_as
    /// In order to support the ability to have a hashtable of strings but
    /// be able to do efficiently lookups via char pointers (i.e. so they 
    /// aren't converted to string objects), we provide the find_as 
    /// function. This function allows you to do a find with a key of a
    /// type other than the hashtable key type.
    ///
    /// Example find_as usage:
    ///     hash_set<string> hashSet;
    ///     i = hashSet.find_as("hello");    // Use default hash and compare.
    ///
    /// Example find_as usage (namespaces omitted for brevity):
    ///     hash_set<string> hashSet;
    ///     i = hashSet.find_as("hello", hash<char*>(), equal_to_2<string, char*>());
    ///
    template <typename Value, typename Hash = eastl::hash<Value>, typename Predicate = eastl::equal_to<Value>, 
              typename Allocator = EASTLAllocatorType, bool bCacheHashCode = false>
    class hash_set
        : public hashtable<Value, Value, Allocator, eastl::use_self<Value>, Predicate,
                           Hash, mod_range_hashing, default_ranged_hash, 
                           prime_rehash_policy, bCacheHashCode, false, true>
    {
    public:
        typedef hashtable<Value, Value, Allocator, eastl::use_self<Value>, Predicate, 
                          Hash, mod_range_hashing, default_ranged_hash,
                          prime_rehash_policy, bCacheHashCode, false, true>       base_type;
        typedef hash_set<Value, Hash, Predicate, Allocator, bCacheHashCode>       this_type;
        typedef typename base_type::size_type                                     size_type;
        typedef typename base_type::value_type                                    value_type;
        typedef typename base_type::allocator_type                                allocator_type;
        typedef typename base_type::node_type                                     node_type;

    public:
        /// hash_set
        ///
        /// Default constructor.
        /// 
        explicit hash_set(const allocator_type& allocator = EASTL_HASH_SET_DEFAULT_ALLOCATOR)
            : base_type(0, Hash(), mod_range_hashing(), default_ranged_hash(), Predicate(), eastl::use_self<Value>(), allocator)
        {
            // Empty
        }


        /// hash_set
        ///
        /// Constructor which creates an empty container, but start with nBucketCount buckets.
        /// We default to a small nBucketCount value, though the user really should manually 
        /// specify an appropriate value in order to prevent memory from being reallocated.
        ///
        explicit hash_set(size_type nBucketCount, const Hash& hashFunction = Hash(), const Predicate& predicate = Predicate(), 
                          const allocator_type& allocator = EASTL_HASH_SET_DEFAULT_ALLOCATOR)
            : base_type(nBucketCount, hashFunction, mod_range_hashing(), default_ranged_hash(), predicate, eastl::use_self<Value>(), allocator)
        {
            // Empty
        }


        /// hash_set
        ///
        /// An input bucket count of <= 1 causes the bucket count to be equal to the number of 
        /// elements in the input range.
        ///
        template <typename FowardIterator>
        hash_set(FowardIterator first, FowardIterator last, size_type nBucketCount = 0, const Hash& hashFunction = Hash(), 
                 const Predicate& predicate = Predicate(), const allocator_type& allocator = EASTL_HASH_SET_DEFAULT_ALLOCATOR)
            : base_type(first, last, nBucketCount, hashFunction, mod_range_hashing(), default_ranged_hash(), predicate, eastl::use_self<Value>(), allocator)
        {
            // Empty
        }

    }; // hash_set






    /// hash_multiset
    ///
    /// Implements a hash_multiset, which is the same thing as a hash_set 
    /// except that contained elements need not be unique. See the documentation 
    /// for hash_set for details.
    ///
    template <typename Value, typename Hash = eastl::hash<Value>, typename Predicate = eastl::equal_to<Value>, 
              typename Allocator = EASTLAllocatorType, bool bCacheHashCode = false>
    class hash_multiset
        : public hashtable<Value, Value, Allocator, eastl::use_self<Value>, Predicate,
                           Hash, mod_range_hashing, default_ranged_hash,
                           prime_rehash_policy, bCacheHashCode, false, false>
    {
    public:
        typedef hashtable<Value, Value, Allocator, eastl::use_self<Value>, Predicate,
                          Hash, mod_range_hashing, default_ranged_hash,
                          prime_rehash_policy, bCacheHashCode, false, false>          base_type;
        typedef hash_multiset<Value, Hash, Predicate, Allocator, bCacheHashCode>      this_type;
        typedef typename base_type::size_type                                         size_type;
        typedef typename base_type::value_type                                        value_type;
        typedef typename base_type::allocator_type                                    allocator_type;
        typedef typename base_type::node_type                                         node_type;

    public:
        /// hash_multiset
        ///
        /// Default constructor.
        /// 
        explicit hash_multiset(const allocator_type& allocator = EASTL_HASH_MULTISET_DEFAULT_ALLOCATOR)
            : base_type(0, Hash(), mod_range_hashing(), default_ranged_hash(), Predicate(), eastl::use_self<Value>(), allocator)
        {
            // Empty
        }


        /// hash_multiset
        ///
        /// Constructor which creates an empty container, but start with nBucketCount buckets.
        /// We default to a small nBucketCount value, though the user really should manually 
        /// specify an appropriate value in order to prevent memory from being reallocated.
        ///
        explicit hash_multiset(size_type nBucketCount, const Hash& hashFunction = Hash(), 
                               const Predicate& predicate = Predicate(), const allocator_type& allocator = EASTL_HASH_MULTISET_DEFAULT_ALLOCATOR)
            : base_type(nBucketCount, hashFunction, mod_range_hashing(), default_ranged_hash(), predicate, eastl::use_self<Value>(), allocator)
        {
            // Empty
        }


        /// hash_multiset
        ///
        /// An input bucket count of <= 1 causes the bucket count to be equal to the number of 
        /// elements in the input range.
        ///
        template <typename FowardIterator>
        hash_multiset(FowardIterator first, FowardIterator last, size_type nBucketCount = 0, const Hash& hashFunction = Hash(), 
                      const Predicate& predicate = Predicate(), const allocator_type& allocator = EASTL_HASH_MULTISET_DEFAULT_ALLOCATOR)
            : base_type(first, last, nBucketCount, hashFunction, mod_range_hashing(), default_ranged_hash(), predicate, eastl::use_self<Value>(), allocator)
        {
            // Empty
        }


    }; // hash_multiset




} // namespace eastl


#endif // Header include guard












