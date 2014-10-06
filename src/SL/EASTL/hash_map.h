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
// EASTL/hash_map.h
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


#ifndef EASTL_HASH_MAP_H
#define EASTL_HASH_MAP_H


#include <EASTL/internal/config.h>
#include <EASTL/internal/hashtable.h>
#include <EASTL/functional.h>
#include <EASTL/utility.h>



namespace eastl
{

    /// EASTL_HASH_MAP_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    ///
    #ifndef EASTL_HASH_MAP_DEFAULT_NAME
        #define EASTL_HASH_MAP_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " hash_map" // Unless the user overrides something, this is "EASTL hash_map".
    #endif


    /// EASTL_HASH_MULTIMAP_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    ///
    #ifndef EASTL_HASH_MULTIMAP_DEFAULT_NAME
        #define EASTL_HASH_MULTIMAP_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " hash_multimap" // Unless the user overrides something, this is "EASTL hash_multimap".
    #endif


    /// EASTL_HASH_MAP_DEFAULT_ALLOCATOR
    ///
    #ifndef EASTL_HASH_MAP_DEFAULT_ALLOCATOR
        #define EASTL_HASH_MAP_DEFAULT_ALLOCATOR allocator_type(EASTL_HASH_MAP_DEFAULT_NAME)
    #endif

    /// EASTL_HASH_MULTIMAP_DEFAULT_ALLOCATOR
    ///
    #ifndef EASTL_HASH_MULTIMAP_DEFAULT_ALLOCATOR
        #define EASTL_HASH_MULTIMAP_DEFAULT_ALLOCATOR allocator_type(EASTL_HASH_MULTIMAP_DEFAULT_NAME)
    #endif



    /// hash_map
    ///
    /// Implements a hash_map, which is a hashed associative container.
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
    ///     hash_map<string, int> hashMap;
    ///     i = hashMap.find_as("hello");    // Use default hash and compare.
    ///
    /// Example find_as usage (namespaces omitted for brevity):
    ///     hash_map<string, int> hashMap;
    ///     i = hashMap.find_as("hello", hash<char*>(), equal_to_2<string, char*>());
    ///
    template <typename Key, typename T, typename Hash = eastl::hash<Key>, typename Predicate = eastl::equal_to<Key>, 
              typename Allocator = EASTLAllocatorType, bool bCacheHashCode = false>
    class hash_map
        : public hashtable<Key, eastl::pair<const Key, T>, Allocator, eastl::use_first<eastl::pair<const Key, T> >, Predicate,
                            Hash, mod_range_hashing, default_ranged_hash, prime_rehash_policy, bCacheHashCode, true, true>
    {
    public:
        typedef hashtable<Key, eastl::pair<const Key, T>, Allocator, 
                          eastl::use_first<eastl::pair<const Key, T> >, 
                          Predicate, Hash, mod_range_hashing, default_ranged_hash, 
                          prime_rehash_policy, bCacheHashCode, true, true>        base_type;
        typedef hash_map<Key, T, Hash, Predicate, Allocator, bCacheHashCode>      this_type;
        typedef typename base_type::size_type                                     size_type;
        typedef typename base_type::key_type                                      key_type;
        typedef T                                                                 mapped_type;
        typedef typename base_type::value_type                                    value_type;     // Note that this is pair<const key_type, mapped_type>.
        typedef typename base_type::allocator_type                                allocator_type;
        typedef typename base_type::node_type                                     node_type;
        typedef typename base_type::insert_return_type                            insert_return_type;
        typedef typename base_type::iterator                                      iterator;

        #if !defined(__GNUC__) || (__GNUC__ >= 3) // GCC 2.x has a bug which we work around.
        using base_type::insert;
        #endif

    public:
        /// hash_map
        ///
        /// Default constructor.
        ///
        explicit hash_map(const allocator_type& allocator = EASTL_HASH_MAP_DEFAULT_ALLOCATOR)
            : base_type(0, Hash(), mod_range_hashing(), default_ranged_hash(), 
                        Predicate(), eastl::use_first<eastl::pair<const Key, T> >(), allocator)
        {
            // Empty
        }


        /// hash_map
        ///
        /// Constructor which creates an empty container, but start with nBucketCount buckets.
        /// We default to a small nBucketCount value, though the user really should manually 
        /// specify an appropriate value in order to prevent memory from being reallocated.
        ///
        explicit hash_map(size_type nBucketCount, const Hash& hashFunction = Hash(), 
                          const Predicate& predicate = Predicate(), const allocator_type& allocator = EASTL_HASH_MAP_DEFAULT_ALLOCATOR)
            : base_type(nBucketCount, hashFunction, mod_range_hashing(), default_ranged_hash(), 
                        predicate, eastl::use_first<eastl::pair<const Key, T> >(), allocator)
        {
            // Empty
        }


        /// hash_map
        ///
        /// An input bucket count of <= 1 causes the bucket count to be equal to the number of 
        /// elements in the input range.
        ///
        template <typename ForwardIterator>
        hash_map(ForwardIterator first, ForwardIterator last, size_type nBucketCount = 0, const Hash& hashFunction = Hash(), 
                 const Predicate& predicate = Predicate(), const allocator_type& allocator = EASTL_HASH_MAP_DEFAULT_ALLOCATOR)
            : base_type(first, last, nBucketCount, hashFunction, mod_range_hashing(), default_ranged_hash(), 
                        predicate, eastl::use_first<eastl::pair<const Key, T> >(), allocator)
        {
            // Empty
        }


        /// insert
        ///
        /// This is an extension to the C++ standard. We insert a default-constructed 
        /// element with the given key. The reason for this is that we can avoid the 
        /// potentially expensive operation of creating and/or copying a mapped_type
        /// object on the stack.
        insert_return_type insert(const key_type& key)
        {
            return base_type::DoInsertKey(key, true_type());
        }


        #if defined(__GNUC__) && (__GNUC__ < 3) // If using old GCC (GCC 2.x has a bug which we work around)
            template <typename InputIterator>
            void               insert(InputIterator first, InputIterator last)    { return base_type::insert(first, last); }
            insert_return_type insert(const value_type& value)                    { return base_type::insert(value);       }
            iterator           insert(const_iterator it, const value_type& value) { return base_type::insert(it, value);   }
        #endif


        mapped_type& operator[](const key_type& key)
        {
            const typename base_type::iterator it = base_type::find(key);
            if(it != base_type::end())
                return (*it).second;
            return (*base_type::insert(value_type(key, mapped_type())).first).second;
        }

    }; // hash_map






    /// hash_multimap
    ///
    /// Implements a hash_multimap, which is the same thing as a hash_map 
    /// except that contained elements need not be unique. See the 
    /// documentation for hash_set for details.
    ///
    template <typename Key, typename T, typename Hash = eastl::hash<Key>, typename Predicate = eastl::equal_to<Key>,
              typename Allocator = EASTLAllocatorType, bool bCacheHashCode = false>
    class hash_multimap
        : public hashtable<Key, eastl::pair<const Key, T>, Allocator, eastl::use_first<eastl::pair<const Key, T> >, Predicate,
                           Hash, mod_range_hashing, default_ranged_hash, prime_rehash_policy, bCacheHashCode, true, false>
    {
    public:
        typedef hashtable<Key, eastl::pair<const Key, T>, Allocator, 
                          eastl::use_first<eastl::pair<const Key, T> >, 
                          Predicate, Hash, mod_range_hashing, default_ranged_hash, 
                          prime_rehash_policy, bCacheHashCode, true, false>           base_type;
        typedef hash_multimap<Key, T, Hash, Predicate, Allocator, bCacheHashCode>     this_type;
        typedef typename base_type::size_type                                         size_type;
        typedef typename base_type::key_type                                          key_type;
        typedef T                                                                     mapped_type;
        typedef typename base_type::value_type                                        value_type;     // Note that this is pair<const key_type, mapped_type>.
        typedef typename base_type::allocator_type                                    allocator_type;
        typedef typename base_type::node_type                                         node_type;
        typedef typename base_type::insert_return_type                                insert_return_type;
        typedef typename base_type::iterator                                          iterator;

        #if !defined(__GNUC__) || (__GNUC__ >= 3) // GCC 2.x has a bug which we work around.
        using base_type::insert;
        #endif

    public:
        /// hash_multimap
        ///
        /// Default constructor.
        ///
        explicit hash_multimap(const allocator_type& allocator = EASTL_HASH_MULTIMAP_DEFAULT_ALLOCATOR)
            : base_type(0, Hash(), mod_range_hashing(), default_ranged_hash(), 
                        Predicate(), eastl::use_first<eastl::pair<const Key, T> >(), allocator)
        {
            // Empty
        }


        /// hash_multimap
        ///
        /// Constructor which creates an empty container, but start with nBucketCount buckets.
        /// We default to a small nBucketCount value, though the user really should manually 
        /// specify an appropriate value in order to prevent memory from being reallocated.
        ///
        explicit hash_multimap(size_type nBucketCount, const Hash& hashFunction = Hash(), 
                               const Predicate& predicate = Predicate(), const allocator_type& allocator = EASTL_HASH_MULTIMAP_DEFAULT_ALLOCATOR)
            : base_type(nBucketCount, hashFunction, mod_range_hashing(), default_ranged_hash(), 
                        predicate, eastl::use_first<eastl::pair<const Key, T> >(), allocator)
        {
            // Empty
        }


        /// hash_multimap
        ///
        /// An input bucket count of <= 1 causes the bucket count to be equal to the number of 
        /// elements in the input range.
        ///
        template <typename ForwardIterator>
        hash_multimap(ForwardIterator first, ForwardIterator last, size_type nBucketCount = 0, const Hash& hashFunction = Hash(), 
                      const Predicate& predicate = Predicate(), const allocator_type& allocator = EASTL_HASH_MULTIMAP_DEFAULT_ALLOCATOR)
            : base_type(first, last, nBucketCount, hashFunction, mod_range_hashing(), default_ranged_hash(), 
                        predicate, eastl::use_first<eastl::pair<const Key, T> >(), allocator)
        {
            // Empty
        }


        /// insert
        ///
        /// This is an extension to the C++ standard. We insert a default-constructed 
        /// element with the given key. The reason for this is that we can avoid the 
        /// potentially expensive operation of creating and/or copying a mapped_type
        /// object on the stack.
        insert_return_type insert(const key_type& key)
        {
            return base_type::DoInsertKey(key, false_type());
        }


        #if defined(__GNUC__) && (__GNUC__ < 3) // If using old GCC (GCC 2.x has a bug which we work around)
            template <typename InputIterator>
            void               insert(InputIterator first, InputIterator last)    { return base_type::insert(first, last); }
            insert_return_type insert(const value_type& value)                    { return base_type::insert(value);       }
            iterator           insert(const_iterator it, const value_type& value) { return base_type::insert(it, value);   }
        #endif


    }; // hash_multimap




} // namespace eastl


#endif // Header include guard






