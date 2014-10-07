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
// EASTL/internal/hashtable.h
//
// Written and maintained by Paul Pedriana - 2005.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements a hashtable, much like the C++ TR1 hash_set/hash_map.
// proposed classes.
// The primary distinctions between this hashtable and TR1 hash tables are:
//    - hashtable is savvy to an environment that doesn't have exception handling,
//      as is sometimes the case with console or embedded environments.
//    - hashtable is slightly more space-efficient than a conventional std hashtable 
//      implementation on platforms with 64 bit size_t. This is 
//      because std STL uses size_t (64 bits) in data structures whereby 32 bits 
//      of data would be fine.
//    - hashtable can contain objects with alignment requirements. TR1 hash tables 
//      cannot do so without a bit of tedious non-portable effort.
//    - hashtable supports debug memory naming natively.
//    - hashtable provides a find function that lets you specify a type that is 
//      different from the hash table key type. This is particularly useful for 
//      the storing of string objects but finding them by char pointers.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file is currently partially based on the TR1 (technical report 1) 
// reference implementation of the hash_set/hash_map C++ classes 
// as of about 4/2005. 
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_INTERNAL_HASHTABLE_H
#define EASTL_INTERNAL_HASHTABLE_H


#include <EASTL/internal/config.h>
#include <EASTL/type_traits.h>
#include <EASTL/allocator.h>
#include <EASTL/iterator.h>
#include <EASTL/functional.h>
#include <EASTL/utility.h>
#include <EASTL/algorithm.h>
#include <string.h>

#ifdef _MSC_VER
    #pragma warning(push, 0)
    //#include <new>
    #include <stddef.h>
    #pragma warning(pop)
#else
    //#include <new>
    #include <stddef.h>
#endif

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4512)  // 'class' : assignment operator could not be generated.
    #pragma warning(disable: 4530)  // C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
#endif


namespace eastl
{

    /// EASTL_HASHTABLE_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    ///
    #ifndef EASTL_HASHTABLE_DEFAULT_NAME
        #define EASTL_HASHTABLE_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " hashtable" // Unless the user overrides something, this is "EASTL hashtable".
    #endif


    /// EASTL_HASHTABLE_DEFAULT_ALLOCATOR
    ///
    #ifndef EASTL_HASHTABLE_DEFAULT_ALLOCATOR
        #define EASTL_HASHTABLE_DEFAULT_ALLOCATOR allocator_type(EASTL_HASHTABLE_DEFAULT_NAME)
    #endif



    /// gpEmptyBucketArray
    ///
    /// A shared representation of an empty hash table. This is present so that
    /// a new empty hashtable allocates no memory. It has two entries, one for 
    /// the first lone empty (NULL) bucket, and one for the non-NULL trailing sentinel.
    /// 
    extern EASTL_API void* gpEmptyBucketArray[2];



    /// hash_node
    ///
    /// A hash_node stores an element in a hash table, much like a 
    /// linked list node stores an element in a linked list. 
    /// A hash_node additionally can, via template parameter,
    /// store a hash code in the node to speed up hash calculations 
    /// and comparisons in some cases.
    /// 
    template <typename Value, bool bCacheHashCode>
    struct hash_node;

    template <typename Value>
    struct hash_node<Value, true>
    {
        Value        mValue;
        hash_node*   mpNext;
        eastl_size_t mnHashCode;      // See config.h for the definition of eastl_size_t, which defaults to uint32_t.
    } EASTL_MAY_ALIAS;

    template <typename Value>
    struct hash_node<Value, false>
    {
        Value      mValue;
        hash_node* mpNext;
    } EASTL_MAY_ALIAS;



    /// node_iterator_base
    ///
    /// Node iterators iterate nodes within a given bucket.
    ///
    /// We define a base class here because it is shared by both const and
    /// non-const iterators.
    ///
    template <typename Value, bool bCacheHashCode>
    struct node_iterator_base
    {
    public:
        typedef hash_node<Value, bCacheHashCode> node_type;

        node_type* mpNode;

    public:
        node_iterator_base(node_type* pNode)
            : mpNode(pNode) { }

        void increment()
            { mpNode = mpNode->mpNext; }
    };



    /// node_iterator
    ///
    /// Node iterators iterate nodes within a given bucket.
    ///
    /// The bConst parameter defines if the iterator is a const_iterator
    /// or an iterator.
    ///
    template <typename Value, bool bConst, bool bCacheHashCode>
    struct node_iterator : public node_iterator_base<Value, bCacheHashCode>
    {
    public:
        typedef node_iterator_base<Value, bCacheHashCode>                base_type;
        typedef node_iterator<Value, bConst, bCacheHashCode>             this_type;
        typedef typename base_type::node_type                            node_type;
        typedef Value                                                    value_type;
        typedef typename type_select<bConst, const Value*, Value*>::type pointer;
        typedef typename type_select<bConst, const Value&, Value&>::type reference;
        typedef ptrdiff_t                                                difference_type;
        typedef EASTL_ITC_NS::forward_iterator_tag                       iterator_category;

    public:
        explicit node_iterator(node_type* pNode = NULL)
            : base_type(pNode) { }

        node_iterator(const node_iterator<Value, true, bCacheHashCode>& x)
            : base_type(x.mpNode) { }

        reference operator*() const
            { return base_type::mpNode->mValue; }

        pointer operator->() const
            { return &(base_type::mpNode->mValue); }

        node_iterator& operator++()
            { base_type::increment(); return *this; }

        node_iterator operator++(int)
            { node_iterator temp(*this); base_type::increment(); return temp; }

    }; // node_iterator



    /// hashtable_iterator_base
    ///
    /// A hashtable_iterator iterates the entire hash table and not just
    /// nodes within a single bucket. Users in general will use a hash
    /// table iterator much more often, as it is much like other container
    /// iterators (e.g. vector::iterator).
    ///
    /// We define a base class here because it is shared by both const and
    /// non-const iterators.
    ///
    template <typename Value, bool bCacheHashCode>
    struct hashtable_iterator_base
    {
    public:
        typedef hash_node<Value, bCacheHashCode> node_type;

    public:
        // We use public here because it allows the hashtable class to access
        // these without a function call, and we are very strongly avoiding 
        // function calls in this library, as our primary goal is performance
        // over correctness and some compilers (e.g. GCC) are terrible at 
        // inlining and so avoiding function calls is of major importance.
        node_type*  mpNode;      // Current node within current bucket.
        node_type** mpBucket;    // Current bucket.

    public:
        hashtable_iterator_base(node_type* pNode, node_type** pBucket)
            : mpNode(pNode), mpBucket(pBucket) { }

        void increment_bucket()
        {
            ++mpBucket;
            while(*mpBucket == NULL) // We store an extra bucket with some non-NULL value at the end 
                ++mpBucket;          // of the bucket array so that finding the end of the bucket
            mpNode = *mpBucket;      // array is quick and simple.
        }

        void increment()
        {
            mpNode = mpNode->mpNext;

            while(mpNode == NULL)
                mpNode = *++mpBucket;
        }

    }; // hashtable_iterator_base




    /// hashtable_iterator
    ///
    /// A hashtable_iterator iterates the entire hash table and not just
    /// nodes within a single bucket. Users in general will use a hash
    /// table iterator much more often, as it is much like other container
    /// iterators (e.g. vector::iterator).
    ///
    /// The bConst parameter defines if the iterator is a const_iterator
    /// or an iterator.
    ///
    template <typename Value, bool bConst, bool bCacheHashCode>
    struct hashtable_iterator : public hashtable_iterator_base<Value, bCacheHashCode>
    {
    public:
        typedef hashtable_iterator_base<Value, bCacheHashCode>           base_type;
        typedef hashtable_iterator<Value, bConst, bCacheHashCode>        this_type;
        typedef hashtable_iterator<Value, false, bCacheHashCode>         this_type_non_const;
        typedef typename base_type::node_type                            node_type;
        typedef Value                                                    value_type;
        typedef typename type_select<bConst, const Value*, Value*>::type pointer;
        typedef typename type_select<bConst, const Value&, Value&>::type reference;
        typedef ptrdiff_t                                                difference_type;
        typedef EASTL_ITC_NS::forward_iterator_tag                       iterator_category;

    public:
        hashtable_iterator(node_type* pNode = NULL, node_type** pBucket = NULL)
            : base_type(pNode, pBucket) { }

        hashtable_iterator(node_type** pBucket)
            : base_type(*pBucket, pBucket) { }

        hashtable_iterator(const this_type_non_const& x)
            : base_type(x.mpNode, x.mpBucket) { }

        reference operator*() const
            { return base_type::mpNode->mValue; }

        pointer operator->() const
            { return &(base_type::mpNode->mValue); }

        hashtable_iterator& operator++()
            { base_type::increment(); return *this; }

        hashtable_iterator operator++(int)
            { hashtable_iterator temp(*this); base_type::increment(); return temp; }

        const node_type* get_node() const
            { return base_type::mpNode; }

    }; // hashtable_iterator




    /// ht_distance
    ///
    /// This function returns the same thing as distance() for 
    /// forward iterators but returns zero for input iterators.
    /// The reason why is that input iterators can only be read
    /// once, and calling distance() on an input iterator destroys
    /// the ability to read it. This ht_distance is used only for
    /// optimization and so the code will merely work better with
    /// forward iterators that input iterators.
    ///
    template <typename Iterator>
    inline typename eastl::iterator_traits<Iterator>::difference_type
    distance_fw_impl(Iterator /* first */, Iterator /* last */, EASTL_ITC_NS::input_iterator_tag)
        { return 0; }

    template <typename Iterator>
    inline typename eastl::iterator_traits<Iterator>::difference_type
    distance_fw_impl(Iterator first, Iterator last, EASTL_ITC_NS::forward_iterator_tag)
        { return eastl::distance(first, last); }

    template <typename Iterator>
    inline typename eastl::iterator_traits<Iterator>::difference_type
    ht_distance(Iterator first, Iterator last)
    {
        typedef typename eastl::iterator_traits<Iterator>::iterator_category IC;
        return distance_fw_impl(first, last, IC());
    }




    /// mod_range_hashing
    ///
    /// Implements the algorithm for conversion of a number in the range of
    /// [0, UINT32_MAX) to the range of [0, BucketCount).
    ///
    struct mod_range_hashing
    {
        // Defined as eastl_size_t instead of size_t because the latter 
        // wastes memory and is sometimes slower on 64 bit machines.
        uint32_t operator()(uint32_t r, uint32_t n) const
            { return r % n; }
    };


    /// default_ranged_hash
    ///
    /// Default ranged hash function H. In principle it should be a
    /// function object composed from objects of type H1 and H2 such that
    /// h(k, n) = h2(h1(k), n), but that would mean making extra copies of
    /// h1 and h2. So instead we'll just use a tag to tell class template
    /// hashtable to do that composition.
    ///
    struct default_ranged_hash{ };


    /// prime_rehash_policy
    ///
    /// Default value for rehash policy. Bucket size is (usually) the
    /// smallest prime that keeps the load factor small enough.
    ///
    struct EASTL_API prime_rehash_policy
    {
    public:
        float            mfMaxLoadFactor;
        float            mfGrowthFactor;
        mutable uint32_t mnNextResize;

    public:
        prime_rehash_policy(float fMaxLoadFactor = 1.f)
            : mfMaxLoadFactor(fMaxLoadFactor), mfGrowthFactor(2.f), mnNextResize(0) { }

        float GetMaxLoadFactor() const
            { return mfMaxLoadFactor; }

        /// Return a bucket count no greater than nBucketCountHint, 
        /// Don't update member variables while at it.
        static uint32_t GetPrevBucketCountOnly(uint32_t nBucketCountHint);

        /// Return a bucket count no greater than nBucketCountHint.
        /// This function has a side effect of updating mnNextResize.
        uint32_t GetPrevBucketCount(uint32_t nBucketCountHint) const;

        /// Return a bucket count no smaller than nBucketCountHint.
        /// This function has a side effect of updating mnNextResize.
        uint32_t GetNextBucketCount(uint32_t nBucketCountHint) const;

        /// Return a bucket count appropriate for nElementCount elements.
        /// This function has a side effect of updating mnNextResize.
        uint32_t GetBucketCount(uint32_t nElementCount) const;

        /// nBucketCount is current bucket count, nElementCount is current element count,
        /// and nElementAdd is number of elements to be inserted. Do we need 
        /// to increase bucket count? If so, return pair(true, n), where 
        /// n is the new bucket count. If not, return pair(false, 0).
        eastl::pair<bool, uint32_t>
        GetRehashRequired(uint32_t nBucketCount, uint32_t nElementCount, uint32_t nElementAdd) const;
    };





    ///////////////////////////////////////////////////////////////////////
    // Base classes for hashtable. We define these base classes because 
    // in some cases we want to do different things depending on the 
    // value of a policy class. In some cases the policy class affects
    // which member functions and nested typedefs are defined; we handle that
    // by specializing base class templates. Several of the base class templates
    // need to access other members of class template hashtable, so we use
    // the "curiously recurring template pattern" (parent class is templated 
    // on type of child class) for them.
    ///////////////////////////////////////////////////////////////////////


    /// rehash_base
    ///
    /// Give hashtable the get_max_load_factor functions if the rehash 
    /// policy is prime_rehash_policy.
    ///
    template <typename RehashPolicy, typename Hashtable>
    struct rehash_base { };

    template <typename Hashtable>
    struct rehash_base<prime_rehash_policy, Hashtable>
    {
        // Returns the max load factor, which is the load factor beyond
        // which we rebuild the container with a new bucket count.
        float get_max_load_factor() const
        {
            const Hashtable* const pThis = static_cast<const Hashtable*>(this);
            return pThis->rehash_policy().GetMaxLoadFactor();
        }

        // If you want to make the hashtable never rehash (resize), 
        // set the max load factor to be a very high number (e.g. 100000.f).
        void set_max_load_factor(float fMaxLoadFactor)
        {
            Hashtable* const pThis = static_cast<Hashtable*>(this);
            pThis->rehash_policy(prime_rehash_policy(fMaxLoadFactor));    
        }
    };




    /// hash_code_base
    ///
    /// Encapsulates two policy issues that aren't quite orthogonal.
    ///   (1) The difference between using a ranged hash function and using
    ///       the combination of a hash function and a range-hashing function.
    ///       In the former case we don't have such things as hash codes, so
    ///       we have a dummy type as placeholder.
    ///   (2) Whether or not we cache hash codes. Caching hash codes is
    ///       meaningless if we have a ranged hash function. This is because
    ///       a ranged hash function converts an object directly to its
    ///       bucket index without ostensibly using a hash code.
    /// We also put the key extraction and equality comparison function 
    /// objects here, for convenience.
    ///
    template <typename Key, typename Value, typename ExtractKey, typename Equal, 
              typename H1, typename H2, typename H, bool bCacheHashCode>
    struct hash_code_base;


    /// hash_code_base
    ///
    /// Specialization: ranged hash function, no caching hash codes. 
    /// H1 and H2 are provided but ignored. We define a dummy hash code type.
    ///
    template <typename Key, typename Value, typename ExtractKey, typename Equal, typename H1, typename H2, typename H>
    struct hash_code_base<Key, Value, ExtractKey, Equal, H1, H2, H, false>
    {
    protected:
        ExtractKey  mExtractKey;    // To do: Make this member go away entirely, as it never has any data.
        Equal       mEqual;         // To do: Make this instance use zero space when it is zero size.
        H           mRangedHash;    // To do: Make this instance use zero space when it is zero size

    public:
        H1 hash_function() const
            { return H1(); }

        Equal equal_function() const // Deprecated. Use key_eq() instead, as key_eq is what the new C++ standard 
            { return mEqual; }       // has specified in its hashtable (unordered_*) proposal.

        const Equal& key_eq() const
            { return mEqual; }

        Equal& key_eq()
            { return mEqual; }

    protected:
        typedef void*    hash_code_t;
        typedef uint32_t bucket_index_t;

        hash_code_base(const ExtractKey& extractKey, const Equal& eq, const H1&, const H2&, const H& h)
            : mExtractKey(extractKey), mEqual(eq), mRangedHash(h) { }

        hash_code_t get_hash_code(const Key& /* key */) const
            { return NULL; }

        bucket_index_t bucket_index(hash_code_t, uint32_t) const
            { return (bucket_index_t)0; }

        bucket_index_t bucket_index(const Key& key, hash_code_t, uint32_t nBucketCount) const
            { return (bucket_index_t)mRangedHash(key, nBucketCount); }

        bucket_index_t bucket_index(const hash_node<Value, false>* pNode, uint32_t nBucketCount) const
            { return (bucket_index_t)mRangedHash(mExtractKey(pNode->mValue), nBucketCount); }

        bool compare(const Key& key, hash_code_t, hash_node<Value, false>* pNode) const
            { return mEqual(key, mExtractKey(pNode->mValue)); }

        void copy_code(hash_node<Value, false>*, const hash_node<Value, false>*) const
            { } // Nothing to do.

        void set_code(hash_node<Value, false>* /* pDest */, hash_code_t /* c */) const
            { } // Nothing to do.

        void base_swap(hash_code_base& x)
        {
            eastl::swap(mExtractKey, x.mExtractKey);
            eastl::swap(mEqual,      x.mEqual);
            eastl::swap(mRangedHash, x.mRangedHash);
        }

    }; // hash_code_base



    // No specialization for ranged hash function while caching hash codes.
    // That combination is meaningless, and trying to do it is an error.


    /// hash_code_base
    ///
    /// Specialization: ranged hash function, cache hash codes. 
    /// This combination is meaningless, so we provide only a declaration
    /// and no definition.
    ///
    template <typename Key, typename Value, typename ExtractKey, typename Equal, typename H1, typename H2, typename H>
    struct hash_code_base<Key, Value, ExtractKey, Equal, H1, H2, H, true>;



    /// hash_code_base
    ///
    /// Specialization: hash function and range-hashing function, 
    /// no caching of hash codes. H is provided but ignored. 
    /// Provides typedef and accessor required by TR1.
    ///
    template <typename Key, typename Value, typename ExtractKey, typename Equal, typename H1, typename H2>
    struct hash_code_base<Key, Value, ExtractKey, Equal, H1, H2, default_ranged_hash, false>
    {
    protected:
        ExtractKey  mExtractKey;
        Equal       mEqual;
        H1          m_h1;
        H2          m_h2;

    public:
        typedef H1 hasher;

        H1 hash_function() const
            { return m_h1; }

        Equal equal_function() const // Deprecated. Use key_eq() instead, as key_eq is what the new C++ standard 
            { return mEqual; }       // has specified in its hashtable (unordered_*) proposal.

        const Equal& key_eq() const
            { return mEqual; }

        Equal& key_eq()
            { return mEqual; }

    protected:
        typedef uint32_t hash_code_t;
        typedef uint32_t bucket_index_t;
        typedef hash_node<Value, false> node_type;

        hash_code_base(const ExtractKey& ex, const Equal& eq, const H1& h1, const H2& h2, const default_ranged_hash&)
            : mExtractKey(ex), mEqual(eq), m_h1(h1), m_h2(h2) { }

        hash_code_t get_hash_code(const Key& key) const
            { return (hash_code_t)m_h1(key); }

        bucket_index_t bucket_index(hash_code_t c, uint32_t nBucketCount) const
            { return (bucket_index_t)m_h2(c, nBucketCount); }

        bucket_index_t bucket_index(const Key&, hash_code_t c, uint32_t nBucketCount) const
            { return (bucket_index_t)m_h2(c, nBucketCount); }

        bucket_index_t bucket_index(const node_type* pNode, uint32_t nBucketCount) const
            { return (bucket_index_t)m_h2((hash_code_t)m_h1(mExtractKey(pNode->mValue)), nBucketCount); }

        bool compare(const Key& key, hash_code_t, node_type* pNode) const
            { return mEqual(key, mExtractKey(pNode->mValue)); }

        void copy_code(node_type*, const node_type*) const
            { } // Nothing to do.

        void set_code(node_type*, hash_code_t) const
            { } // Nothing to do.

        void base_swap(hash_code_base& x)
        {
            eastl::swap(mExtractKey, x.mExtractKey);
            eastl::swap(mEqual,      x.mEqual);
            eastl::swap(m_h1,        x.m_h1);
            eastl::swap(m_h2,        x.m_h2);
        }

    }; // hash_code_base



    /// hash_code_base
    ///
    /// Specialization: hash function and range-hashing function, 
    /// caching hash codes. H is provided but ignored. 
    /// Provides typedef and accessor required by TR1.
    ///
    template <typename Key, typename Value, typename ExtractKey, typename Equal, typename H1, typename H2>
    struct hash_code_base<Key, Value, ExtractKey, Equal, H1, H2, default_ranged_hash, true>
    {
    protected:
        ExtractKey  mExtractKey;
        Equal       mEqual;
        H1          m_h1;
        H2          m_h2;

    public:
        typedef H1 hasher;

        H1 hash_function() const
            { return m_h1; }

        Equal equal_function() const // Deprecated. Use key_eq() instead, as key_eq is what the new C++ standard 
            { return mEqual; }       // has specified in its hashtable (unordered_*) proposal.

        const Equal& key_eq() const
            { return mEqual; }

        Equal& key_eq()
            { return mEqual; }

    protected:
        typedef uint32_t hash_code_t;
        typedef uint32_t bucket_index_t;
        typedef hash_node<Value, true> node_type;

        hash_code_base(const ExtractKey& ex, const Equal& eq, const H1& h1, const H2& h2, const default_ranged_hash&)
            : mExtractKey(ex), mEqual(eq), m_h1(h1), m_h2(h2) { }

        hash_code_t get_hash_code(const Key& key) const
            { return (hash_code_t)m_h1(key); }

        bucket_index_t bucket_index(hash_code_t c, uint32_t nBucketCount) const
            { return (bucket_index_t)m_h2(c, nBucketCount); }

        bucket_index_t bucket_index(const Key&, hash_code_t c, uint32_t nBucketCount) const
            { return (bucket_index_t)m_h2(c, nBucketCount); }

        bucket_index_t bucket_index(const node_type* pNode, uint32_t nBucketCount) const
            { return (bucket_index_t)m_h2((uint32_t)pNode->mnHashCode, nBucketCount); }

        bool compare(const Key& key, hash_code_t c, node_type* pNode) const
            { return (pNode->mnHashCode == c) && mEqual(key, mExtractKey(pNode->mValue)); }

        void copy_code(node_type* pDest, const node_type* pSource) const
            { pDest->mnHashCode = pSource->mnHashCode; }

        void set_code(node_type* pDest, hash_code_t c) const
            { pDest->mnHashCode = c; }

        void base_swap(hash_code_base& x)
        {
            eastl::swap(mExtractKey, x.mExtractKey);
            eastl::swap(mEqual,      x.mEqual);
            eastl::swap(m_h1,        x.m_h1);
            eastl::swap(m_h2,        x.m_h2);
        }

    }; // hash_code_base





    ///////////////////////////////////////////////////////////////////////////
    /// hashtable
    ///
    /// Key and Value: arbitrary CopyConstructible types.
    ///
    /// ExtractKey: function object that takes a object of type Value
    /// and returns a value of type Key.
    ///
    /// Equal: function object that takes two objects of type k and returns
    /// a bool-like value that is true if the two objects are considered equal.
    ///
    /// H1: a hash function. A unary function object with argument type
    /// Key and result type size_t. Return values should be distributed
    /// over the entire range [0, numeric_limits<uint32_t>::max()].
    ///
    /// H2: a range-hashing function (in the terminology of Tavori and
    /// Dreizin). This is a function which takes the output of H1 and 
    /// converts it to the range of [0, n]. Usually it merely takes the
    /// output of H1 and mods it to n.
    ///
    /// H: a ranged hash function (Tavori and Dreizin). This is merely
    /// a class that combines the functionality of H1 and H2 together, 
    /// possibly in some way that is somehow improved over H1 and H2
    /// It is a binary function whose argument types are Key and size_t 
    /// and whose result type is uint32_t. Given arguments k and n, the 
    /// return value is in the range [0, n). Default: h(k, n) = h2(h1(k), n). 
    /// If H is anything other than the default, H1 and H2 are ignored, 
    /// as H is thus overriding H1 and H2.
    ///
    /// RehashPolicy: Policy class with three members, all of which govern
    /// the bucket count. nBucket(n) returns a bucket count no smaller
    /// than n. GetBucketCount(n) returns a bucket count appropriate
    /// for an element count of n. GetRehashRequired(nBucketCount, nElementCount, nElementAdd)
    /// determines whether, if the current bucket count is nBucket and the
    /// current element count is nElementCount, we need to increase the bucket
    /// count. If so, returns pair(true, n), where n is the new
    /// bucket count. If not, returns pair(false, <anything>).
    ///
    /// Currently it is hard-wired that the number of buckets never
    /// shrinks. Should we allow RehashPolicy to change that?
    ///
    /// bCacheHashCode: true if we store the value of the hash
    /// function along with the value. This is a time-space tradeoff.
    /// Storing it may improve lookup speed by reducing the number of 
    /// times we need to call the Equal function.
    ///
    /// bMutableIterators: true if hashtable::iterator is a mutable
    /// iterator, false if iterator and const_iterator are both const 
    /// iterators. This is true for hash_map and hash_multimap,
    /// false for hash_set and hash_multiset.
    ///
    /// bUniqueKeys: true if the return value of hashtable::count(k)
    /// is always at most one, false if it may be an arbitrary number. 
    /// This is true for hash_set and hash_map and is false for 
    /// hash_multiset and hash_multimap.
    ///
    ///////////////////////////////////////////////////////////////////////
    /// Note:
    /// If you want to make a hashtable never increase its bucket usage,
    /// call set_max_load_factor with a very high value such as 100000.f.
    ///
    /// find_as
    /// In order to support the ability to have a hashtable of strings but
    /// be able to do efficiently lookups via char pointers (i.e. so they 
    /// aren't converted to string objects), we provide the find_as 
    /// function. This function allows you to do a find with a key of a
    /// type other than the hashtable key type. See the find_as function
    /// for more documentation on this.
    ///
    /// find_by_hash
    /// In the interest of supporting fast operations wherever possible,
    /// we provide a find_by_hash function which finds a node using its
    /// hash code.  This is useful for cases where the node's hash is
    /// already known, allowing us to avoid a redundant hash operation
    /// in the normal find path.
    /// 
    template <typename Key, typename Value, typename Allocator, typename ExtractKey, 
              typename Equal, typename H1, typename H2, typename H, 
              typename RehashPolicy, bool bCacheHashCode, bool bMutableIterators, bool bUniqueKeys>
    class hashtable
        :   public rehash_base<RehashPolicy, hashtable<Key, Value, Allocator, ExtractKey, Equal, H1, H2, H, RehashPolicy, bCacheHashCode, bMutableIterators, bUniqueKeys> >,
            public hash_code_base<Key, Value, ExtractKey, Equal, H1, H2, H, bCacheHashCode>
    {
    public:
        typedef Key                                                                                 key_type;
        typedef Value                                                                               value_type;
        typedef typename ExtractKey::result_type                                                    mapped_type;
        typedef hash_code_base<Key, Value, ExtractKey, Equal, H1, H2, H, bCacheHashCode>            hash_code_base_type;
        typedef typename hash_code_base_type::hash_code_t                                           hash_code_t;
        typedef Allocator                                                                           allocator_type;
        typedef Equal                                                                               key_equal;
        typedef ptrdiff_t                                                                           difference_type;
        typedef eastl_size_t                                                                        size_type;     // See config.h for the definition of eastl_size_t, which defaults to uint32_t.
        typedef value_type&                                                                         reference;
        typedef const value_type&                                                                   const_reference;
        typedef node_iterator<value_type, !bMutableIterators, bCacheHashCode>                       local_iterator;
        typedef node_iterator<value_type, true,               bCacheHashCode>                       const_local_iterator;
        typedef hashtable_iterator<value_type, !bMutableIterators, bCacheHashCode>                  iterator;
        typedef hashtable_iterator<value_type, true,               bCacheHashCode>                  const_iterator;
        typedef eastl::reverse_iterator<iterator>                                                   reverse_iterator;
        typedef eastl::reverse_iterator<const_iterator>                                             const_reverse_iterator;
        typedef hash_node<value_type, bCacheHashCode>                                               node_type;
        typedef typename type_select<bUniqueKeys, eastl::pair<iterator, bool>, iterator>::type      insert_return_type;
        typedef hashtable<Key, Value, Allocator, ExtractKey, Equal, H1, H2, H, 
                            RehashPolicy, bCacheHashCode, bMutableIterators, bUniqueKeys>           this_type;
        typedef RehashPolicy                                                                        rehash_policy_type;
        typedef ExtractKey                                                                          extract_key_type;
        typedef H1                                                                                  h1_type;
        typedef H2                                                                                  h2_type;
        typedef H                                                                                   h_type;

        using hash_code_base_type::key_eq;
        using hash_code_base_type::hash_function;
        using hash_code_base_type::mExtractKey;
        using hash_code_base_type::get_hash_code;
        using hash_code_base_type::bucket_index;
        using hash_code_base_type::compare;
        using hash_code_base_type::set_code;

        static const bool kCacheHashCode = bCacheHashCode;

        enum
        {
            kKeyAlignment         = EASTL_ALIGN_OF(key_type),
            kKeyAlignmentOffset   = 0,                          // To do: Make sure this really is zero for all uses of this template.
            kValueAlignment       = EASTL_ALIGN_OF(value_type),
            kValueAlignmentOffset = 0,                          // To fix: This offset is zero for sets and >0 for maps. Need to fix this.
            kAllocFlagBuckets     = 0x00400000                  // Flag to allocator which indicates that we are allocating buckets and not nodes.
        };

    protected:
        node_type**     mpBucketArray;
        size_type       mnBucketCount;
        size_type       mnElementCount;
        RehashPolicy    mRehashPolicy;  // To do: Use base class optimization to make this go away.
        allocator_type  mAllocator;     // To do: Use base class optimization to make this go away.

    public:
        hashtable(size_type nBucketCount, const H1&, const H2&, const H&, const Equal&, const ExtractKey&, 
                  const allocator_type& allocator = EASTL_HASHTABLE_DEFAULT_ALLOCATOR);
        
        template <typename FowardIterator>
        hashtable(FowardIterator first, FowardIterator last, size_type nBucketCount, 
                  const H1&, const H2&, const H&, const Equal&, const ExtractKey&, 
                  const allocator_type& allocator = EASTL_HASHTABLE_DEFAULT_ALLOCATOR); // allocator arg removed because VC7.1 fails on the default arg. To do: Make a second version of this function without a default arg.
        
        hashtable(const hashtable& x);
       ~hashtable();

        allocator_type& get_allocator();
        void            set_allocator(const allocator_type& allocator);

        this_type& operator=(const this_type& x);

        void swap(this_type& x);

    public:
        iterator begin()
        {
            iterator i(mpBucketArray);
            if(!i.mpNode)
                i.increment_bucket();
            return i;
        }

        const_iterator begin() const
        {
            const_iterator i(mpBucketArray);
            if(!i.mpNode)
                i.increment_bucket();
            return i;
        }

        iterator end()
            { return iterator(mpBucketArray + mnBucketCount); }

        const_iterator end() const
            { return const_iterator(mpBucketArray + mnBucketCount); }

        local_iterator begin(size_type n)
            { return local_iterator(mpBucketArray[n]); }

        local_iterator end(size_type)
            { return local_iterator(NULL); }

        const_local_iterator begin(size_type n) const
            { return const_local_iterator(mpBucketArray[n]); }

        const_local_iterator end(size_type) const
            { return const_local_iterator(NULL); }

        bool empty() const
        { return mnElementCount == 0; }

        size_type size() const
        { return mnElementCount; }

        size_type bucket_count() const
        { return mnBucketCount; }

        size_type bucket_size(size_type n) const
        { return (size_type)eastl::distance(begin(n), end(n)); }

        //size_type bucket(const key_type& k) const
        //    { return bucket_index(k, (hash code here), (uint32_t)mnBucketCount); }

    public:
        float load_factor() const
            { return (float)mnElementCount / (float)mnBucketCount; }

        // Inherited from the base class.
        // Returns the max load factor, which is the load factor beyond
        // which we rebuild the container with a new bucket count.
        // get_max_load_factor comes from rehash_base.
        //    float get_max_load_factor() const;

        // Inherited from the base class.
        // If you want to make the hashtable never rehash (resize), 
        // set the max load factor to be a very high number (e.g. 100000.f).
        // set_max_load_factor comes from rehash_base.
        //    void set_max_load_factor(float fMaxLoadFactor);

        /// Generalization of get_max_load_factor. This is an extension that's
        /// not present in TR1.
        const rehash_policy_type& rehash_policy() const
            { return mRehashPolicy; }

        /// Generalization of set_max_load_factor. This is an extension that's
        /// not present in TR1.
        void rehash_policy(const rehash_policy_type& rehashPolicy);

    public:

        insert_return_type insert(const value_type& value);
        iterator           insert(const_iterator, const value_type& value);

        template <typename InputIterator>
        void insert(InputIterator first, InputIterator last);

    public:
        iterator         erase(iterator position);
        iterator         erase(iterator first, iterator last);
        reverse_iterator erase(reverse_iterator position);
        reverse_iterator erase(reverse_iterator first, reverse_iterator last);
        size_type        erase(const key_type& k);

        void clear();
        void clear(bool clearBuckets);
        void reset();
        void rehash(size_type nBucketCount);

    public:
        iterator       find(const key_type& key);
        const_iterator find(const key_type& key) const;

        /// Implements a find whereby the user supplies a comparison of a different type
        /// than the hashtable value_type. A useful case of this is one whereby you have
        /// a container of string objects but want to do searches via passing in char pointers.
        /// The problem is that without this kind of find, you need to do the expensive operation
        /// of converting the char pointer to a string so it can be used as the argument to the 
        /// find function.
        ///
        /// Example usage (namespaces omitted for brevity):
        ///     hash_set<string> hashSet;
        ///     hashSet.find_as("hello");    // Use default hash and compare.
        ///
        /// Example usage (note that the predicate uses string as first type and char* as second):
        ///     hash_set<string> hashSet;
        ///     hashSet.find_as("hello", hash<char*>(), equal_to_2<string, char*>());
        ///
        template <typename U, typename UHash, typename BinaryPredicate>
        iterator       find_as(const U& u, UHash uhash, BinaryPredicate predicate);

        template <typename U, typename UHash, typename BinaryPredicate>
        const_iterator find_as(const U& u, UHash uhash, BinaryPredicate predicate) const;

        template <typename U>
        iterator       find_as(const U& u);

        template <typename U>
        const_iterator find_as(const U& u) const;

        /// Implements a find whereby the user supplies the node's hash code.
        ///
        iterator       find_by_hash(hash_code_t c);
        const_iterator find_by_hash(hash_code_t c) const;

        size_type      count(const key_type& k) const;

        eastl::pair<iterator, iterator>             equal_range(const key_type& k);
        eastl::pair<const_iterator, const_iterator> equal_range(const key_type& k) const;

    public:
        bool validate() const;
        int  validate_iterator(const_iterator i) const;

    protected:
        node_type*  DoAllocateNode(const value_type& value);
        node_type*  DoAllocateNodeFromKey(const key_type& key);
        void        DoFreeNode(node_type* pNode);
        void        DoFreeNodes(node_type** pBucketArray, size_type);

        node_type** DoAllocateBuckets(size_type n);
        void        DoFreeBuckets(node_type** pBucketArray, size_type n);

        eastl::pair<iterator, bool>        DoInsertValue(const value_type& value, true_type);
        iterator                           DoInsertValue(const value_type& value, false_type);

        eastl::pair<iterator, bool>        DoInsertKey(const key_type& key, true_type);
        iterator                           DoInsertKey(const key_type& key, false_type);

        void       DoRehash(size_type nBucketCount);
        node_type* DoFindNode(node_type* pNode, const key_type& k, hash_code_t c) const;

        template <typename U, typename BinaryPredicate>
        node_type* DoFindNode(node_type* pNode, const U& u, BinaryPredicate predicate) const;

        node_type* DoFindNode(node_type* pNode, hash_code_t c) const;

    }; // class hashtable





    ///////////////////////////////////////////////////////////////////////
    // node_iterator_base
    ///////////////////////////////////////////////////////////////////////

    template <typename Value, bool bCacheHashCode>
    inline bool operator==(const node_iterator_base<Value, bCacheHashCode>& a, const node_iterator_base<Value, bCacheHashCode>& b)
        { return a.mpNode == b.mpNode; }

    template <typename Value, bool bCacheHashCode>
    inline bool operator!=(const node_iterator_base<Value, bCacheHashCode>& a, const node_iterator_base<Value, bCacheHashCode>& b)
        { return a.mpNode != b.mpNode; }




    ///////////////////////////////////////////////////////////////////////
    // hashtable_iterator_base
    ///////////////////////////////////////////////////////////////////////

    template <typename Value, bool bCacheHashCode>
    inline bool operator==(const hashtable_iterator_base<Value, bCacheHashCode>& a, const hashtable_iterator_base<Value, bCacheHashCode>& b)
        { return a.mpNode == b.mpNode; }

    template <typename Value, bool bCacheHashCode>
    inline bool operator!=(const hashtable_iterator_base<Value, bCacheHashCode>& a, const hashtable_iterator_base<Value, bCacheHashCode>& b)
        { return a.mpNode != b.mpNode; }




    ///////////////////////////////////////////////////////////////////////
    // hashtable
    ///////////////////////////////////////////////////////////////////////

    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>
    ::hashtable(size_type nBucketCount, const H1& h1, const H2& h2, const H& h,
                const Eq& eq, const EK& ek, const allocator_type& allocator)
        :   rehash_base<RP, hashtable>(),
            hash_code_base<K, V, EK, Eq, H1, H2, H, bC>(ek, eq, h1, h2, h),
            mnBucketCount(0),
            mnElementCount(0),
            mRehashPolicy(),
            mAllocator(allocator)
    {
        if(nBucketCount < 2)  // If we are starting in an initially empty state, with no memory allocation done.
            reset();
        else // Else we are creating a potentially non-empty hashtable...
        {
            EASTL_ASSERT(nBucketCount < 10000000);
            mnBucketCount = (size_type)mRehashPolicy.GetNextBucketCount((uint32_t)nBucketCount);
            mpBucketArray = DoAllocateBuckets(mnBucketCount); // mnBucketCount will always be at least 2.
        }
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    template <typename FowardIterator>
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::hashtable(FowardIterator first, FowardIterator last, size_type nBucketCount, 
                                                                     const H1& h1, const H2& h2, const H& h, 
                                                                     const Eq& eq, const EK& ek, const allocator_type& allocator)
        :   rehash_base<rehash_policy_type, hashtable>(),
            hash_code_base<key_type, value_type, extract_key_type, key_equal, h1_type, h2_type, h_type, kCacheHashCode>(ek, eq, h1, h2, h),
          //mnBucketCount(0), // This gets re-assigned below.
            mnElementCount(0),
            mRehashPolicy(),
            mAllocator(allocator)
    {
        if(nBucketCount < 2)
        {
            const size_type nElementCount = (size_type)eastl::ht_distance(first, last);
            mnBucketCount = (size_type)mRehashPolicy.GetBucketCount((uint32_t)nElementCount);
        }
        else
        {
            EASTL_ASSERT(nBucketCount < 10000000);
            mnBucketCount = nBucketCount;
        }

        mpBucketArray = DoAllocateBuckets(mnBucketCount); // mnBucketCount will always be at least 2.

        #if EASTL_EXCEPTIONS_ENABLED
            try
            {
        #endif
                for(; first != last; ++first)
                    insert(*first);
        #if EASTL_EXCEPTIONS_ENABLED
            }
            catch(...)
            {
                clear();
                DoFreeBuckets(mpBucketArray, mnBucketCount);
                throw;
            }
        #endif
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::hashtable(const this_type& x)
        :   rehash_base<RP, hashtable>(x),
            hash_code_base<K, V, EK, Eq, H1, H2, H, bC>(x),
            mnBucketCount(x.mnBucketCount),
            mnElementCount(x.mnElementCount),
            mRehashPolicy(x.mRehashPolicy),
            mAllocator(x.mAllocator)
    {
        if(mnElementCount) // If there is anything to copy...
        {
            mpBucketArray = DoAllocateBuckets(mnBucketCount); // mnBucketCount will be at least 2.

            #if EASTL_EXCEPTIONS_ENABLED
                try
                {
            #endif
                    for(size_type i = 0; i < x.mnBucketCount; ++i)
                    {
                        node_type*  pNodeSource = x.mpBucketArray[i];
                        node_type** ppNodeDest  = mpBucketArray + i;

                        while(pNodeSource)
                        {
                            *ppNodeDest = DoAllocateNode(pNodeSource->mValue);
                            this->copy_code(*ppNodeDest, pNodeSource);
                            ppNodeDest = &(*ppNodeDest)->mpNext;
                            pNodeSource = pNodeSource->mpNext;
                        }
                    }
            #if EASTL_EXCEPTIONS_ENABLED
                }
                catch(...)
                {
                    clear();
                    DoFreeBuckets(mpBucketArray, mnBucketCount);
                    throw;
                }
            #endif
        }
        else
        {
            // In this case, instead of allocate memory and copy nothing from x, 
            // we reset ourselves to a zero allocation state.
            reset();
        }
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::allocator_type&
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::get_allocator()
    {
        return mAllocator;
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline void hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::set_allocator(const allocator_type& allocator)
    {
        mAllocator = allocator;
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::this_type&
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::operator=(const this_type& x)
    {
        if(this != &x)
        {
            clear();

            #if EASTL_ALLOCATOR_COPY_ENABLED
                mAllocator = x.mAllocator;
            #endif

            insert(x.begin(), x.end());
        }
        return *this;
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::~hashtable()
    {
        clear();
        DoFreeBuckets(mpBucketArray, mnBucketCount);
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::node_type*
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoAllocateNode(const value_type& value)
    {
        node_type* const pNode = (node_type*)allocate_memory(mAllocator, sizeof(node_type), kValueAlignment, kValueAlignmentOffset);

        #if EASTL_EXCEPTIONS_ENABLED
            try
            {
        #endif
                ::new(&pNode->mValue) value_type(value);
                pNode->mpNext = NULL;
                return pNode;
        #if EASTL_EXCEPTIONS_ENABLED
            }
            catch(...)
            {
                EASTLFree(mAllocator, pNode, sizeof(node_type));
                throw;
            }
        #endif
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::node_type*
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoAllocateNodeFromKey(const key_type& key)
    {
        node_type* const pNode = (node_type*)allocate_memory(mAllocator, sizeof(node_type), kValueAlignment, kValueAlignmentOffset);

        #if EASTL_EXCEPTIONS_ENABLED
            try
            {
        #endif
                ::new(&pNode->mValue) value_type(key);
                pNode->mpNext = NULL;
                return pNode;
        #if EASTL_EXCEPTIONS_ENABLED
            }
            catch(...)
            {
                EASTLFree(mAllocator, pNode, sizeof(node_type));
                throw;
            }
        #endif
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline void hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoFreeNode(node_type* pNode)
    {
        pNode->~node_type();
        EASTLFree(mAllocator, pNode, sizeof(node_type));
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    void hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoFreeNodes(node_type** pNodeArray, size_type n)
    {
        for(size_type i = 0; i < n; ++i)
        {
            node_type* pNode = pNodeArray[i];
            while(pNode)
            {
                node_type* const pTempNode = pNode;
                pNode = pNode->mpNext;
                DoFreeNode(pTempNode);
            }
            pNodeArray[i] = NULL;
        }
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::node_type**
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoAllocateBuckets(size_type n)
    {
        // We allocate one extra bucket to hold a sentinel, an arbitrary
        // non-null pointer. Iterator increment relies on this.
        EASTL_ASSERT(n > 1); // We reserve an mnBucketCount of 1 for the shared gpEmptyBucketArray.
        EASTL_CT_ASSERT(kAllocFlagBuckets == 0x00400000); // Currently we expect this to be so, because the allocator has a copy of this enum.
        node_type** const pBucketArray = (node_type**)EASTLAllocFlags(mAllocator, (n + 1) * sizeof(node_type*), kAllocFlagBuckets);
        //eastl::fill(pBucketArray, pBucketArray + n, (node_type*)NULL);
        memset(pBucketArray, 0, n * sizeof(node_type*));
        pBucketArray[n] = reinterpret_cast<node_type*>((uintptr_t)~0);
        return pBucketArray;
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline void hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoFreeBuckets(node_type** pBucketArray, size_type n)
    {
        // If n <= 1, then pBucketArray is from the shared gpEmptyBucketArray. We don't test 
        // for pBucketArray == &gpEmptyBucketArray because one library have a different gpEmptyBucketArray
        // than another but pass a hashtable to another. So we go by the size.
        if(n > 1)
            EASTLFree(mAllocator, pBucketArray, (n + 1) * sizeof(node_type*)); // '+1' because DoAllocateBuckets allocates nBucketCount + 1 buckets in order to have a NULL sentinel at the end.
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    void hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::swap(this_type& x)
    {
        if(mAllocator == x.mAllocator) // If allocators are equivalent...
        {
            // We leave mAllocator as-is.
            hash_code_base<K, V, EK, Eq, H1, H2, H, bC>::base_swap(x); // hash_code_base has multiple implementations, so we let them handle the swap.
            eastl::swap(mRehashPolicy,  x.mRehashPolicy);
            eastl::swap(mpBucketArray,  x.mpBucketArray);
            eastl::swap(mnBucketCount,  x.mnBucketCount);
            eastl::swap(mnElementCount, x.mnElementCount);
        }
        else
        {
            const this_type temp(*this); // Can't call eastl::swap because that would
            *this = x;                   // itself call this member swap function.
            x     = temp;
        }
    }


    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline void hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::rehash_policy(const rehash_policy_type& rehashPolicy)
    {
        mRehashPolicy = rehashPolicy;

        const size_type nBuckets = rehashPolicy.GetBucketCount((uint32_t)mnElementCount);

        if(nBuckets > mnBucketCount)
            DoRehash(nBuckets);
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::find(const key_type& k)
    {
        const hash_code_t c = get_hash_code(k);
        const size_type   n = (size_type)bucket_index(k, c, (uint32_t)mnBucketCount);

        node_type* const pNode = DoFindNode(mpBucketArray[n], k, c);
        return pNode ? iterator(pNode, mpBucketArray + n) : iterator(mpBucketArray + mnBucketCount); // iterator(mpBucketArray + mnBucketCount) == end()
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::const_iterator
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::find(const key_type& k) const
    {
        const hash_code_t c = get_hash_code(k);
        const size_type   n = (size_type)bucket_index(k, c, (uint32_t)mnBucketCount);

        node_type* const pNode = DoFindNode(mpBucketArray[n], k, c);
        return pNode ? const_iterator(pNode, mpBucketArray + n) : const_iterator(mpBucketArray + mnBucketCount); // iterator(mpBucketArray + mnBucketCount) == end()
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    template <typename U, typename UHash, typename BinaryPredicate>
    inline typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::find_as(const U& other, UHash uhash, BinaryPredicate predicate)
    {
        const hash_code_t c = (hash_code_t)uhash(other);
        const size_type   n = (size_type)(c % mnBucketCount); // This assumes we are using the mod range policy.

        node_type* const pNode = DoFindNode(mpBucketArray[n], other, predicate);
        return pNode ? iterator(pNode, mpBucketArray + n) : iterator(mpBucketArray + mnBucketCount); // iterator(mpBucketArray + mnBucketCount) == end()
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    template <typename U, typename UHash, typename BinaryPredicate>
    inline typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::const_iterator
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::find_as(const U& other, UHash uhash, BinaryPredicate predicate) const
    {
        const hash_code_t c = (hash_code_t)uhash(other);
        const size_type   n = (size_type)(c % mnBucketCount); // This assumes we are using the mod range policy.

        node_type* const pNode = DoFindNode(mpBucketArray[n], other, predicate);
        return pNode ? const_iterator(pNode, mpBucketArray + n) : const_iterator(mpBucketArray + mnBucketCount); // iterator(mpBucketArray + mnBucketCount) == end()
    }


    /// hashtable_find
    ///
    /// Helper function that defaults to using hash<U> and equal_to_2<T, U>.
    /// This makes it so that by default you don't need to provide these.
    /// Note that the default hash functions may not be what you want, though.
    ///
    /// Example usage. Instead of this:
    ///     hash_set<string> hashSet;
    ///     hashSet.find("hello", hash<char*>(), equal_to_2<string, char*>());
    ///
    /// You can use this:
    ///     hash_set<string> hashSet;
    ///     hashtable_find(hashSet, "hello");
    ///
    template <typename H, typename U>
    inline typename H::iterator hashtable_find(H& hashTable, U u)
        { return hashTable.find_as(u, eastl::hash<U>(), eastl::equal_to_2<const typename H::key_type, U>()); }

    template <typename H, typename U>
    inline typename H::const_iterator hashtable_find(const H& hashTable, U u)
        { return hashTable.find_as(u, eastl::hash<U>(), eastl::equal_to_2<const typename H::key_type, U>()); }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    template <typename U>
    inline typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::find_as(const U& other)
        { return eastl::hashtable_find(*this, other); }
        // VC++ doesn't appear to like the following, though it seems correct to me.
        // So we implement the workaround above until we can straighten this out.
        //{ return find_as(other, eastl::hash<U>(), eastl::equal_to_2<const key_type, U>()); }


    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    template <typename U>
    inline typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::const_iterator
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::find_as(const U& other) const
        { return eastl::hashtable_find(*this, other); }
        // VC++ doesn't appear to like the following, though it seems correct to me.
        // So we implement the workaround above until we can straighten this out.
        //{ return find_as(other, eastl::hash<U>(), eastl::equal_to_2<const key_type, U>()); }


    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::find_by_hash(hash_code_t c)
    {
        const size_type n = (size_type)bucket_index(c, (uint32_t)mnBucketCount);

        node_type* const pNode = DoFindNode(mpBucketArray[n], c);
        return pNode ? iterator(pNode, mpBucketArray + n) : iterator(mpBucketArray + mnBucketCount); // iterator(mpBucketArray + mnBucketCount) == end()
    }


    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::const_iterator
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::find_by_hash(hash_code_t c) const
    {
        const size_type n = (size_type)bucket_index(c, (uint32_t)mnBucketCount);

        node_type* const pNode = DoFindNode(mpBucketArray[n], c);
        return pNode ? const_iterator(pNode, mpBucketArray + n) : const_iterator(mpBucketArray + mnBucketCount); // iterator(mpBucketArray + mnBucketCount) == end()
    }


    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::size_type
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::count(const key_type& k) const
    {
        const hash_code_t c      = get_hash_code(k);
        const size_type   n      = (size_type)bucket_index(k, c, (uint32_t)mnBucketCount);
        size_type         result = 0;

        // To do: Make a specialization for bU (unique keys) == true and take 
        // advantage of the fact that the count will always be zero or one in that case. 
        for(node_type* pNode = mpBucketArray[n]; pNode; pNode = pNode->mpNext)
        {
            if(compare(k, c, pNode))
                ++result;
        }
        return result;
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    eastl::pair<typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator,
                typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator>
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::equal_range(const key_type& k)
    {
        const hash_code_t c = get_hash_code(k);
        const size_type   n = (size_type)bucket_index(k, c, (uint32_t)mnBucketCount);

        node_type** head  = mpBucketArray + n;
        node_type*  pNode = DoFindNode(*head, k, c);

        if(pNode)
        {
            node_type* p1 = pNode->mpNext;

            for(; p1; p1 = p1->mpNext)
            {
                if(!compare(k, c, p1))
                    break;
            }

            iterator first(pNode, head);
            iterator last(p1, head);

            if(!p1)
                last.increment_bucket();

            return eastl::pair<iterator, iterator>(first, last);
        }

        return eastl::pair<iterator, iterator>(iterator(mpBucketArray + mnBucketCount),  // iterator(mpBucketArray + mnBucketCount) == end()
                                               iterator(mpBucketArray + mnBucketCount));
    }




    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    eastl::pair<typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::const_iterator,
                typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::const_iterator>
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::equal_range(const key_type& k) const
    {
        const hash_code_t c     = get_hash_code(k);
        const size_type   n     = (size_type)bucket_index(k, c, (uint32_t)mnBucketCount);
        node_type**       head  = mpBucketArray + n;
        node_type*        pNode = DoFindNode(*head, k, c);

        if(pNode)
        {
            node_type* p1 = pNode->mpNext;

            for(; p1; p1 = p1->mpNext)
            {
                if(!compare(k, c, p1))
                    break;
            }

            const_iterator first(pNode, head);
            const_iterator last(p1, head);

            if(!p1)
                last.increment_bucket();

            return eastl::pair<const_iterator, const_iterator>(first, last);
        }

        return eastl::pair<const_iterator, const_iterator>(const_iterator(mpBucketArray + mnBucketCount),  // iterator(mpBucketArray + mnBucketCount) == end()
                                                           const_iterator(mpBucketArray + mnBucketCount));
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::node_type* 
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoFindNode(node_type* pNode, const key_type& k, hash_code_t c) const
    {
        for(; pNode; pNode = pNode->mpNext)
        {
            if(compare(k, c, pNode))
                return pNode;
        }
        return NULL;
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    template <typename U, typename BinaryPredicate>
    inline typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::node_type* 
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoFindNode(node_type* pNode, const U& other, BinaryPredicate predicate) const
    {
        for(; pNode; pNode = pNode->mpNext)
        {
            if(predicate(mExtractKey(pNode->mValue), other)) // Intentionally compare with key as first arg and other as second arg.
                return pNode;
        }
        return NULL;
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::node_type* 
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoFindNode(node_type* pNode, hash_code_t c) const
    {
        for(; pNode; pNode = pNode->mpNext)
        {
            if(pNode->mnHashCode == c)
                return pNode;
        }
        return NULL;
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    eastl::pair<typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator, bool>
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoInsertValue(const value_type& value, true_type) // true_type means bUniqueKeys is true.
    {
        const key_type&   k     = mExtractKey(value);
        const hash_code_t c     = get_hash_code(k);
        size_type         n     = (size_type)bucket_index(k, c, (uint32_t)mnBucketCount);
        node_type* const  pNode = DoFindNode(mpBucketArray[n], k, c);

        if(pNode == NULL)
        {
            const eastl::pair<bool, uint32_t> bRehash = mRehashPolicy.GetRehashRequired((uint32_t)mnBucketCount, (uint32_t)mnElementCount, (uint32_t)1);

            // Allocate the new node before doing the rehash so that we don't
            // do a rehash if the allocation throws.
            node_type* const pNodeNew = DoAllocateNode(value);
            set_code(pNodeNew, c); // This is a no-op for most hashtables.

            #if EASTL_EXCEPTIONS_ENABLED
                try
                {
            #endif
                    if(bRehash.first)
                    {
                        n = (size_type)bucket_index(k, c, (uint32_t)bRehash.second);
                        DoRehash(bRehash.second);
                    }

                    EASTL_ASSERT((void**)mpBucketArray != &gpEmptyBucketArray[0]);
                    pNodeNew->mpNext = mpBucketArray[n];
                    mpBucketArray[n] = pNodeNew;
                    ++mnElementCount;

                    return eastl::pair<iterator, bool>(iterator(pNodeNew, mpBucketArray + n), true);
            #if EASTL_EXCEPTIONS_ENABLED
                }
                catch(...)
                {
                    DoFreeNode(pNodeNew);
                    throw;
                }
            #endif
        }

        return eastl::pair<iterator, bool>(iterator(pNode, mpBucketArray + n), false);
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoInsertValue(const value_type& value, false_type) // false_type means bUniqueKeys is false.
    {
        const eastl::pair<bool, uint32_t> bRehash = mRehashPolicy.GetRehashRequired((uint32_t)mnBucketCount, (uint32_t)mnElementCount, (uint32_t)1);

        if(bRehash.first)
            DoRehash(bRehash.second);

        const key_type&   k = mExtractKey(value);
        const hash_code_t c = get_hash_code(k);
        const size_type   n = (size_type)bucket_index(k, c, (uint32_t)mnBucketCount);

        node_type* const pNodeNew = DoAllocateNode(value);
        set_code(pNodeNew, c); // This is a no-op for most hashtables.

        // To consider: Possibly make this insertion not make equal elements contiguous.
        // As it stands now, we insert equal values contiguously in the hashtable.
        // The benefit is that equal_range can work in a sensible manner and that
        // erase(value) can more quickly find equal values. The downside is that
        // this insertion operation taking some extra time. How important is it to
        // us that equal_range span all equal items? 
        node_type* const pNodePrev = DoFindNode(mpBucketArray[n], k, c);

        if(pNodePrev == NULL)
        {
            EASTL_ASSERT((void**)mpBucketArray != &gpEmptyBucketArray[0]);
            pNodeNew->mpNext = mpBucketArray[n];
            mpBucketArray[n] = pNodeNew;
        }
        else
        {
            pNodeNew->mpNext  = pNodePrev->mpNext;
            pNodePrev->mpNext = pNodeNew;
        }

        ++mnElementCount;

        return iterator(pNodeNew, mpBucketArray + n);
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    eastl::pair<typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator, bool>
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoInsertKey(const key_type& key, true_type) // true_type means bUniqueKeys is true.
    {
        const hash_code_t c     = get_hash_code(key);
        size_type         n     = (size_type)bucket_index(key, c, (uint32_t)mnBucketCount);
        node_type* const  pNode = DoFindNode(mpBucketArray[n], key, c);

        if(pNode == NULL)
        {
            const eastl::pair<bool, uint32_t> bRehash = mRehashPolicy.GetRehashRequired((uint32_t)mnBucketCount, (uint32_t)mnElementCount, (uint32_t)1);

            // Allocate the new node before doing the rehash so that we don't
            // do a rehash if the allocation throws.
            node_type* const pNodeNew = DoAllocateNodeFromKey(key);
            set_code(pNodeNew, c); // This is a no-op for most hashtables.

            #if EASTL_EXCEPTIONS_ENABLED
                try
                {
            #endif
                    if(bRehash.first)
                    {
                        n = (size_type)bucket_index(key, c, (uint32_t)bRehash.second);
                        DoRehash(bRehash.second);
                    }

                    EASTL_ASSERT((void**)mpBucketArray != &gpEmptyBucketArray[0]);
                    pNodeNew->mpNext = mpBucketArray[n];
                    mpBucketArray[n] = pNodeNew;
                    ++mnElementCount;

                    return eastl::pair<iterator, bool>(iterator(pNodeNew, mpBucketArray + n), true);
            #if EASTL_EXCEPTIONS_ENABLED
                }
                catch(...)
                {
                    DoFreeNode(pNodeNew);
                    throw;
                }
            #endif
        }

        return eastl::pair<iterator, bool>(iterator(pNode, mpBucketArray + n), false);
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoInsertKey(const key_type& key, false_type) // false_type means bUniqueKeys is false.
    {
        const eastl::pair<bool, uint32_t> bRehash = mRehashPolicy.GetRehashRequired((uint32_t)mnBucketCount, (uint32_t)mnElementCount, (uint32_t)1);

        if(bRehash.first)
            DoRehash(bRehash.second);

        const hash_code_t c = get_hash_code(key);
        const size_type   n = (size_type)bucket_index(key, c, (uint32_t)mnBucketCount);

        node_type* const pNodeNew = DoAllocateNodeFromKey(key);
        set_code(pNodeNew, c); // This is a no-op for most hashtables.

        // To consider: Possibly make this insertion not make equal elements contiguous.
        // As it stands now, we insert equal values contiguously in the hashtable.
        // The benefit is that equal_range can work in a sensible manner and that
        // erase(value) can more quickly find equal values. The downside is that
        // this insertion operation taking some extra time. How important is it to
        // us that equal_range span all equal items? 
        node_type* const pNodePrev = DoFindNode(mpBucketArray[n], key, c);

        if(pNodePrev == NULL)
        {
            EASTL_ASSERT((void**)mpBucketArray != &gpEmptyBucketArray[0]);
            pNodeNew->mpNext = mpBucketArray[n];
            mpBucketArray[n] = pNodeNew;
        }
        else
        {
            pNodeNew->mpNext  = pNodePrev->mpNext;
            pNodePrev->mpNext = pNodeNew;
        }

        ++mnElementCount;

        return iterator(pNodeNew, mpBucketArray + n);
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert_return_type
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert(const value_type& value) 
    {
        return DoInsertValue(value, integral_constant<bool, bU>());
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert(const_iterator, const value_type& value)
    {
        // We ignore the first argument (hint iterator). It's not likely to be useful for hashtable containers.

        #ifdef __MWERKS__ // The Metrowerks compiler has a bug.
            insert_return_type result = insert(value);
            return result.first; // Note by Paul Pedriana while perusing this code: This code will fail to compile when bU is false (i.e. for multiset, multimap).

        #elif defined(__GNUC__) && (__GNUC__ < 3) // If using old GCC (GCC 2.x has a bug which we work around)
            EASTL_ASSERT(empty()); // This function cannot return the correct return value on GCC 2.x. Unless, that is, the container is empty.
            DoInsertValue(value, integral_constant<bool, bU>());
            return begin(); // This is the wrong answer.
        #else
            insert_return_type result = DoInsertValue(value, integral_constant<bool, bU>());
            return result.first; // Note by Paul Pedriana while perusing this code: This code will fail to compile when bU is false (i.e. for multiset, multimap).
        #endif
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    template <typename InputIterator>
    void
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::insert(InputIterator first, InputIterator last)
    {
        const uint32_t nElementAdd = (uint32_t)eastl::ht_distance(first, last);
        const eastl::pair<bool, uint32_t> bRehash = mRehashPolicy.GetRehashRequired((uint32_t)mnBucketCount, (uint32_t)mnElementCount, nElementAdd);

        if(bRehash.first)
            DoRehash(bRehash.second);

        for(; first != last; ++first)
        {
            #ifdef __MWERKS__ // The Metrowerks compiler has a bug.
                insert(*first);
            #else
                DoInsertValue(*first, integral_constant<bool, bU>());
            #endif
        }
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::erase(iterator i)
    {
        iterator iNext(i);
        ++iNext;

        node_type* pNode        =  i.mpNode;
        node_type* pNodeCurrent = *i.mpBucket;

        if(pNodeCurrent == pNode)
            *i.mpBucket = pNodeCurrent->mpNext;
        else
        {
            // We have a singly-linked list, so we have no choice but to
            // walk down it till we find the node before the node at 'i'.
            node_type* pNodeNext = pNodeCurrent->mpNext;

            while(pNodeNext != pNode)
            {
                pNodeCurrent = pNodeNext;
                pNodeNext    = pNodeCurrent->mpNext;
            }

            pNodeCurrent->mpNext = pNodeNext->mpNext;
        }

        DoFreeNode(pNode);
        --mnElementCount;

        return iNext;
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::iterator
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::erase(iterator first, iterator last)
    {
        while(first != last)
            first = erase(first);
        return first;
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::reverse_iterator
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::erase(reverse_iterator position)
    {
        return reverse_iterator(erase((++position).base()));
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::reverse_iterator
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::erase(reverse_iterator first, reverse_iterator last)
    {
        // Version which erases in order from first to last.
        // difference_type i(first.base() - last.base());
        // while(i--)
        //     first = erase(first);
        // return first;

        // Version which erases in order from last to first, but is slightly more efficient:
        return reverse_iterator(erase((++last).base(), (++first).base()));
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    typename hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::size_type 
    hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::erase(const key_type& k)
    {
        // To do: Reimplement this function to do a single loop and not try to be 
        // smart about element contiguity. The mechanism here is only a benefit if the 
        // buckets are heavily overloaded; otherwise this mechanism may be slightly slower.

        const hash_code_t c = get_hash_code(k);
        const size_type   n = (size_type)bucket_index(k, c, (uint32_t)mnBucketCount);
        const size_type   nElementCountSaved = mnElementCount;

        node_type** pBucketArray = mpBucketArray + n;

        while(*pBucketArray && !compare(k, c, *pBucketArray))
            pBucketArray = &(*pBucketArray)->mpNext;

        while(*pBucketArray && compare(k, c, *pBucketArray))
        {
            node_type* const pNode = *pBucketArray;
            *pBucketArray = pNode->mpNext;
            DoFreeNode(pNode);
            --mnElementCount;
        }

        return nElementCountSaved - mnElementCount;
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline void hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::clear()
    {
        DoFreeNodes(mpBucketArray, mnBucketCount);
        mnElementCount = 0;
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline void hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::clear(bool clearBuckets)
    {
        DoFreeNodes(mpBucketArray, mnBucketCount);
        if(clearBuckets)
        {
            DoFreeBuckets(mpBucketArray, mnBucketCount);
            reset();
        }
        mnElementCount = 0;
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline void hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::reset()
    {
        // The reset function is a special extension function which unilaterally 
        // resets the container to an empty state without freeing the memory of 
        // the contained objects. This is useful for very quickly tearing down a 
        // container built into scratch memory.
        mnBucketCount  = 1;

        #ifdef _MSC_VER
            mpBucketArray = (node_type**)&gpEmptyBucketArray[0];
        #else
            void* p = &gpEmptyBucketArray[0];
            memcpy(&mpBucketArray, &p, sizeof(mpBucketArray)); // Other compilers implement strict aliasing and casting is thus unsafe.
        #endif

        mnElementCount = 0;
        mRehashPolicy.mnNextResize = 0;
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline void hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::rehash(size_type nBucketCount)
    {
        // Note that we unilaterally use the passed in bucket count; we do not attempt migrate it
        // up to the next prime number. We leave it at the user's discretion to do such a thing.
        DoRehash(nBucketCount);
    }



    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    void hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::DoRehash(size_type nNewBucketCount)
    {
        node_type** const pBucketArray = DoAllocateBuckets(nNewBucketCount); // nNewBucketCount should always be >= 2.

        #if EASTL_EXCEPTIONS_ENABLED
            try
            {
        #endif
                node_type* pNode;

                for(size_type i = 0; i < mnBucketCount; ++i)
                {
                    while((pNode = mpBucketArray[i]) != NULL) // Using '!=' disables compiler warnings.
                    {
                        const size_type nNewBucketIndex = (size_type)bucket_index(pNode, (uint32_t)nNewBucketCount);

                        mpBucketArray[i] = pNode->mpNext;
                        pNode->mpNext    = pBucketArray[nNewBucketIndex];
                        pBucketArray[nNewBucketIndex] = pNode;
                    }
                }

                DoFreeBuckets(mpBucketArray, mnBucketCount);
                mnBucketCount = nNewBucketCount;
                mpBucketArray = pBucketArray;
        #if EASTL_EXCEPTIONS_ENABLED
            }
            catch(...)
            {
                // A failure here means that a hash function threw an exception.
                // We can't restore the previous state without calling the hash
                // function again, so the only sensible recovery is to delete everything.
                DoFreeNodes(pBucketArray, nNewBucketCount);
                DoFreeBuckets(pBucketArray, nNewBucketCount);
                DoFreeNodes(mpBucketArray, mnBucketCount);
                mnElementCount = 0;
                throw;
            }
        #endif
    }


    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline bool hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::validate() const
    {
        // Verify our empty bucket array is unmodified.
        if(gpEmptyBucketArray[0] != NULL)
            return false;

        if(gpEmptyBucketArray[1] != (void*)uintptr_t(~0))
            return false;

        // Verify that we have at least one bucket. Calculations can  
        // trigger division by zero exceptions otherwise.
        if(mnBucketCount == 0)
            return false;

        // Verify that gpEmptyBucketArray is used correctly.
        // gpEmptyBucketArray is only used when initially empty.
        if((void**)mpBucketArray == &gpEmptyBucketArray[0])
        {
            if(mnElementCount) // gpEmptyBucketArray is used only for empty hash tables.
                return false;

            if(mnBucketCount != 1) // gpEmptyBucketArray is used exactly an only for mnBucketCount == 1.
                return false;
        }
        else
        {
            if(mnBucketCount < 2) // Small bucket counts *must* use gpEmptyBucketArray.
                return false;
        }

        // Verify that the element count matches mnElementCount. 
        size_type nElementCount = 0;

        for(const_iterator temp = begin(), tempEnd = end(); temp != tempEnd; ++temp)
            ++nElementCount;

        if(nElementCount != mnElementCount)
            return false;

        // To do: Verify that individual elements are in the expected buckets.

        return true;
    }


    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    int hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>::validate_iterator(const_iterator i) const
    {
        // To do: Come up with a more efficient mechanism of doing this.

        for(const_iterator temp = begin(), tempEnd = end(); temp != tempEnd; ++temp)
        {
            if(temp == i)
                return (isf_valid | isf_current | isf_can_dereference);
        }

        if(i == end())
            return (isf_valid | isf_current); 

        return isf_none;
    }



    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline bool operator==(const hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>& a, 
                           const hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>& b)
    {
        return (a.size() == b.size()) && eastl::equal(a.begin(), a.end(), b.begin());
    }


    // Comparing hash tables for less-ness is an odd thing to do. We provide it for 
    // completeness, though the user is advised to be wary of how they use this.
    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline bool operator<(const hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>& a, 
                          const hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>& b)
    {
        // This requires hash table elements to support operator<. Since the hash table
        // doesn't compare elements via less (it does so via equals), we must use the 
        // globally defined operator less for the elements.
        return eastl::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
    }


    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline bool operator!=(const hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>& a, 
                           const hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>& b)
    {
        return !(a == b);
    }


    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline bool operator>(const hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>& a, 
                          const hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>& b)
    {
        return b < a;
    }


    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline bool operator<=(const hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>& a, 
                           const hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>& b)
    {
        return !(b < a);
    }


    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline bool operator>=(const hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>& a, 
                           const hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>& b)
    {
        return !(a < b);
    }


    template <typename K, typename V, typename A, typename EK, typename Eq,
              typename H1, typename H2, typename H, typename RP, bool bC, bool bM, bool bU>
    inline void swap(const hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>& a, 
                     const hashtable<K, V, A, EK, Eq, H1, H2, H, RP, bC, bM, bU>& b)
    {
        a.swap(b);
    }


} // namespace eastl


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard









