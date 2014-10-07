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
// EASTL/red_black_tree.h
// Written by Paul Pedriana 2005.
//////////////////////////////////////////////////////////////////////////////



#ifndef EASTL_RED_BLACK_TREE_H
#define EASTL_RED_BLACK_TREE_H



#include <EASTL/internal/config.h>
#include <EASTL/type_traits.h>
#include <EASTL/allocator.h>
#include <EASTL/iterator.h>
#include <EASTL/utility.h>
#include <EASTL/algorithm.h>

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
    #pragma warning(disable: 4512)  // 'class' : assignment operator could not be generated
    #pragma warning(disable: 4530)  // C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
#endif


namespace eastl
{

    /// EASTL_RBTREE_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    ///
    #ifndef EASTL_RBTREE_DEFAULT_NAME
        #define EASTL_RBTREE_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " rbtree" // Unless the user overrides something, this is "EASTL rbtree".
    #endif


    /// EASTL_RBTREE_DEFAULT_ALLOCATOR
    ///
    #ifndef EASTL_RBTREE_DEFAULT_ALLOCATOR
        #define EASTL_RBTREE_DEFAULT_ALLOCATOR allocator_type(EASTL_RBTREE_DEFAULT_NAME)
    #endif



    /// RBTreeColor
    ///
    enum RBTreeColor
    {
        kRBTreeColorRed,
        kRBTreeColorBlack
    };



    /// RBTreeColor
    ///
    enum RBTreeSide
    {
        kRBTreeSideLeft,
        kRBTreeSideRight
    };



    /// rbtree_node_base
    ///
    /// We define a rbtree_node_base separately from rbtree_node (below), because it 
    /// allows us to have non-templated operations, and it makes it so that the 
    /// rbtree anchor node doesn't carry a T with it, which would waste space and 
    /// possibly lead to surprising the user due to extra Ts existing that the user 
    /// didn't explicitly create. The downside to all of this is that it makes debug 
    /// viewing of an rbtree harder, given that the node pointers are of type 
    /// rbtree_node_base and not rbtree_node.
    ///
    struct rbtree_node_base
    {
        typedef rbtree_node_base this_type;

    public:
        this_type* mpNodeRight;  // Declared first because it is used most often.
        this_type* mpNodeLeft;
        this_type* mpNodeParent;
        char       mColor;       // We only need one bit here, would be nice if we could stuff that bit somewhere else.
    };


    /// rbtree_node
    ///
    template <typename Value>
    struct rbtree_node : public rbtree_node_base
    {
        Value mValue; // For set and multiset, this is the user's value, for map and multimap, this is a pair of key/value.
    };




    // rbtree_node_base functions
    //
    // These are the fundamental functions that we use to maintain the 
    // tree. The bulk of the work of the tree maintenance is done in 
    // these functions.
    //
    EASTL_API rbtree_node_base* RBTreeIncrement    (const rbtree_node_base* pNode);
    EASTL_API rbtree_node_base* RBTreeDecrement    (const rbtree_node_base* pNode);
    EASTL_API rbtree_node_base* RBTreeGetMinChild  (const rbtree_node_base* pNode);
    EASTL_API rbtree_node_base* RBTreeGetMaxChild  (const rbtree_node_base* pNode);
    EASTL_API size_t            RBTreeGetBlackCount(const rbtree_node_base* pNodeTop,
                                                    const rbtree_node_base* pNodeBottom);
    EASTL_API void              RBTreeInsert       (      rbtree_node_base* pNode,
                                                          rbtree_node_base* pNodeParent, 
                                                          rbtree_node_base* pNodeAnchor,
                                                          RBTreeSide insertionSide);
    EASTL_API void              RBTreeErase        (      rbtree_node_base* pNode,
                                                          rbtree_node_base* pNodeAnchor); 







    /// rbtree_iterator
    ///
    template <typename T, typename Pointer, typename Reference>
    struct rbtree_iterator
    {
        typedef rbtree_iterator<T, Pointer, Reference>      this_type;
        typedef rbtree_iterator<T, T*, T&>                  iterator;
        typedef rbtree_iterator<T, const T*, const T&>      const_iterator;
        typedef eastl_size_t                                size_type;     // See config.h for the definition of eastl_size_t, which defaults to uint32_t.
        typedef ptrdiff_t                                   difference_type;
        typedef T                                           value_type;
        typedef rbtree_node_base                            base_node_type;
        typedef rbtree_node<T>                              node_type;
        typedef Pointer                                     pointer;
        typedef Reference                                   reference;
        typedef EASTL_ITC_NS::bidirectional_iterator_tag    iterator_category;

    public:
        node_type* mpNode;

    public:
        rbtree_iterator();
        explicit rbtree_iterator(const node_type* pNode);
        rbtree_iterator(const iterator& x);

        reference operator*() const;
        pointer   operator->() const;

        rbtree_iterator& operator++();
        rbtree_iterator  operator++(int);

        rbtree_iterator& operator--();
        rbtree_iterator  operator--(int);

    }; // rbtree_iterator





    ///////////////////////////////////////////////////////////////////////////////
    // rb_base
    //
    // This class allows us to use a generic rbtree as the basis of map, multimap,
    // set, and multiset transparently. The vital template parameters for this are 
    // the ExtractKey and the bUniqueKeys parameters.
    //
    // If the rbtree has a value type of the form pair<T1, T2> (i.e. it is a map or
    // multimap and not a set or multiset) and a key extraction policy that returns 
    // the first part of the pair, the rbtree gets a mapped_type typedef. 
    // If it satisfies those criteria and also has unique keys, then it also gets an 
    // operator[] (which only map and set have and multimap and multiset don't have).
    //
    ///////////////////////////////////////////////////////////////////////////////



    /// rb_base
    /// This specialization is used for 'set'. In this case, Key and Value 
    /// will be the same as each other and ExtractKey will be eastl::use_self.
    ///
    template <typename Key, typename Value, typename Compare, typename ExtractKey, bool bUniqueKeys, typename RBTree>
    struct rb_base
    {
        typedef ExtractKey extract_key;

    public:
        Compare mCompare; // To do: Make sure that empty Compare classes go away via empty base optimizations.

    public:
        rb_base() : mCompare() {}
        rb_base(const Compare& compare) : mCompare(compare) {}
    };


    /// rb_base
    /// This class is used for 'multiset'.
    /// In this case, Key and Value will be the same as each 
    /// other and ExtractKey will be eastl::use_self.
    ///
    template <typename Key, typename Value, typename Compare, typename ExtractKey, typename RBTree>
    struct rb_base<Key, Value, Compare, ExtractKey, false, RBTree>
    {
        typedef ExtractKey extract_key;

    public:
        Compare mCompare; // To do: Make sure that empty Compare classes go away via empty base optimizations.

    public:
        rb_base() : mCompare() {}
        rb_base(const Compare& compare) : mCompare(compare) {}
    };


    /// rb_base
    /// This specialization is used for 'map'.
    ///
    template <typename Key, typename Pair, typename Compare, typename RBTree>
    struct rb_base<Key, Pair, Compare, eastl::use_first<Pair>, true, RBTree>
    {
        typedef eastl::use_first<Pair> extract_key;

    public:
        Compare mCompare; // To do: Make sure that empty Compare classes go away via empty base optimizations.

    public:
        rb_base() : mCompare() {}
        rb_base(const Compare& compare) : mCompare(compare) {}
    };


    /// rb_base
    /// This specialization is used for 'multimap'.
    ///
    template <typename Key, typename Pair, typename Compare, typename RBTree>
    struct rb_base<Key, Pair, Compare, eastl::use_first<Pair>, false, RBTree>
    {
        typedef eastl::use_first<Pair> extract_key;

    public:
        Compare mCompare; // To do: Make sure that empty Compare classes go away via empty base optimizations.

    public:
        rb_base() : mCompare() {}
        rb_base(const Compare& compare) : mCompare(compare) {}
    };





    /// rbtree
    ///
    /// rbtree is the red-black tree basis for the map, multimap, set, and multiset 
    /// containers. Just about all the work of those containers is done here, and 
    /// they are merely a shell which sets template policies that govern the code
    /// generation for this rbtree.
    ///
    /// This rbtree implementation is pretty much the same as all other modern 
    /// rbtree implementations, as the topic is well known and researched. We may
    /// choose to implement a "relaxed balancing" option at some point in the 
    /// future if it is deemed worthwhile. Most rbtree implementations don't do this.
    ///
    /// The primary rbtree member variable is mAnchor, which is a node_type and 
    /// acts as the end node. However, like any other node, it has mpNodeLeft,
    /// mpNodeRight, and mpNodeParent members. We do the conventional trick of 
    /// assigning begin() (left-most rbtree node) to mpNodeLeft, assigning 
    /// 'end() - 1' (a.k.a. rbegin()) to mpNodeRight, and assigning the tree root
    /// node to mpNodeParent. 
    ///
    /// Compare (functor): This is a comparison class which defaults to 'less'.
    /// It is a common STL thing which takes two arguments and returns true if  
    /// the first is less than the second.
    ///
    /// ExtractKey (functor): This is a class which gets the key from a stored
    /// node. With map and set, the node is a pair, whereas with set and multiset
    /// the node is just the value. ExtractKey will be either eastl::use_first (map and multimap)
    /// or eastl::use_self (set and multiset).
    ///
    /// bMutableIterators (bool): true if rbtree::iterator is a mutable
    /// iterator, false if iterator and const_iterator are both const iterators. 
    /// It will be true for map and multimap and false for set and multiset.
    ///
    /// bUniqueKeys (bool): true if the keys are to be unique, and false if there
    /// can be multiple instances of a given key. It will be true for set and map 
    /// and false for multiset and multimap.
    ///
    /// To consider: Add an option for relaxed tree balancing. This could result 
    /// in performance improvements but would require a more complicated implementation.
    ///
    ///////////////////////////////////////////////////////////////////////
    /// find_as
    /// In order to support the ability to have a tree of strings but
    /// be able to do efficiently lookups via char pointers (i.e. so they
    /// aren't converted to string objects), we provide the find_as
    /// function. This function allows you to do a find with a key of a
    /// type other than the tree's key type. See the find_as function
    /// for more documentation on this.
    ///
    template <typename Key, typename Value, typename Compare, typename Allocator, 
              typename ExtractKey, bool bMutableIterators, bool bUniqueKeys>
    class rbtree
        : public rb_base<Key, Value, Compare, ExtractKey, bUniqueKeys, 
                            rbtree<Key, Value, Compare, Allocator, ExtractKey, bMutableIterators, bUniqueKeys> >
    {
    public:
        typedef ptrdiff_t                                                                       difference_type;
        typedef eastl_size_t                                                                    size_type;     // See config.h for the definition of eastl_size_t, which defaults to uint32_t.
        typedef Key                                                                             key_type;
        typedef Value                                                                           value_type;
        typedef rbtree_node<value_type>                                                         node_type;
        typedef value_type&                                                                     reference;
        typedef const value_type&                                                               const_reference;
        typedef typename type_select<bMutableIterators, 
                    rbtree_iterator<value_type, value_type*, value_type&>, 
                    rbtree_iterator<value_type, const value_type*, const value_type&> >::type   iterator;
        typedef rbtree_iterator<value_type, const value_type*, const value_type&>               const_iterator;
        typedef eastl::reverse_iterator<iterator>                                               reverse_iterator;
        typedef eastl::reverse_iterator<const_iterator>                                         const_reverse_iterator;

        typedef Allocator                                                                       allocator_type;
        typedef Compare                                                                         key_compare;
        typedef typename type_select<bUniqueKeys, eastl::pair<iterator, bool>, iterator>::type  insert_return_type;  // map/set::insert return a pair, multimap/multiset::iterator return an iterator.
        typedef rbtree<Key, Value, Compare, Allocator, 
                        ExtractKey, bMutableIterators, bUniqueKeys>                             this_type;
        typedef rb_base<Key, Value, Compare, ExtractKey, bUniqueKeys, this_type>                base_type;
        typedef integral_constant<bool, bUniqueKeys>                                            has_unique_keys_type;
        typedef typename base_type::extract_key                                                 extract_key;

        using base_type::mCompare;

        enum
        {
            kKeyAlignment         = EASTL_ALIGN_OF(key_type),
            kKeyAlignmentOffset   = 0,                          // To do: Make sure this really is zero for all uses of this template.
            kValueAlignment       = EASTL_ALIGN_OF(value_type),
            kValueAlignmentOffset = 0                           // To fix: This offset is zero for sets and >0 for maps. Need to fix this.
        };

    public:
        rbtree_node_base  mAnchor;      /// This node acts as end() and its mpLeft points to begin(), and mpRight points to rbegin() (the last node on the right).
        size_type         mnSize;       /// Stores the count of nodes in the tree (not counting the anchor node).
        allocator_type    mAllocator;   // To do: Use base class optimization to make this go away.

    public:
        // ctor/dtor
        rbtree();
        rbtree(const allocator_type& allocator);
        rbtree(const Compare& compare, const allocator_type& allocator = EASTL_RBTREE_DEFAULT_ALLOCATOR);
        rbtree(const this_type& x);

        template <typename InputIterator>
        rbtree(InputIterator first, InputIterator last, const Compare& compare, const allocator_type& allocator = EASTL_RBTREE_DEFAULT_ALLOCATOR);

       ~rbtree();

    public:
        // properties
        allocator_type& get_allocator();
        void            set_allocator(const allocator_type& allocator);

        const key_compare& key_comp() const { return mCompare; }
        key_compare&       key_comp()       { return mCompare; }

        this_type& operator=(const this_type& x);

        void swap(this_type& x);

    public: 
        // iterators
        iterator        begin();
        const_iterator  begin() const;
        iterator        end();
        const_iterator  end() const;

        reverse_iterator        rbegin();
        const_reverse_iterator  rbegin() const;
        reverse_iterator        rend();
        const_reverse_iterator  rend() const;

    public:
        bool      empty() const;
        size_type size() const;

        /// map::insert and set::insert return a pair, while multimap::insert and
        /// multiset::insert return an iterator.
        insert_return_type insert(const value_type& value);

        // C++ standard: inserts value if and only if there is no element with 
        // key equivalent to the key of t in containers with unique keys; always 
        // inserts value in containers with equivalent keys. Always returns the 
        // iterator pointing to the element with key equivalent to the key of value. 
        // iterator position is a hint pointing to where the insert should start
        // to search. However, there is a potential defect/improvement report on this behaviour:
        // LWG issue #233 (http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2005/n1780.html)
        // We follow the same approach as SGI STL/STLPort and use the position as
        // a forced insertion position for the value when possible.
        iterator insert(iterator position, const value_type& value);

        template <typename InputIterator>
        void insert(InputIterator first, InputIterator last);

        iterator erase(iterator position);
        iterator erase(iterator first, iterator last);

        reverse_iterator erase(reverse_iterator position);
        reverse_iterator erase(reverse_iterator first, reverse_iterator last);

        // For some reason, multiple STL versions make a specialization 
        // for erasing an array of key_types. I'm pretty sure we don't
        // need this, but just to be safe we will follow suit. 
        // The implementation is trivial. Returns void because the values
        // could well be randomly distributed throughout the tree and thus
        // a return value would be nearly meaningless.
        void erase(const key_type* first, const key_type* last);

        void clear();
        void reset();

        iterator       find(const key_type& key);
        const_iterator find(const key_type& key) const;

        /// Implements a find whereby the user supplies a comparison of a different type
        /// than the tree's value_type. A useful case of this is one whereby you have
        /// a container of string objects but want to do searches via passing in char pointers.
        /// The problem is that without this kind of find, you need to do the expensive operation
        /// of converting the char pointer to a string so it can be used as the argument to the
        /// find function.
        ///
        /// Example usage (note that the compare uses string as first type and char* as second):
        ///     set<string> strings;
        ///     strings.find_as("hello", less_2<string, const char*>());
        ///
        template <typename U, typename Compare2>
        iterator       find_as(const U& u, Compare2 compare2);

        template <typename U, typename Compare2>
        const_iterator find_as(const U& u, Compare2 compare2) const;

        iterator       lower_bound(const key_type& key);
        const_iterator lower_bound(const key_type& key) const;

        iterator       upper_bound(const key_type& key);
        const_iterator upper_bound(const key_type& key) const;

        bool validate() const;
        int  validate_iterator(const_iterator i) const;

    protected:
        node_type*  DoAllocateNode();
        void        DoFreeNode(node_type* pNode);

        node_type* DoCreateNodeFromKey(const key_type& key);
        node_type* DoCreateNode(const value_type& value);
        node_type* DoCreateNode(const node_type* pNodeSource, node_type* pNodeParent);

        node_type* DoCopySubtree(const node_type* pNodeSource, node_type* pNodeDest);
        void       DoNukeSubtree(node_type* pNode);

        // Intentionally return a pair and not an iterator for DoInsertValue(..., true_type)
        // This is because the C++ standard for map and set is to return a pair and not just an iterator.
        eastl::pair<iterator, bool> DoInsertValue(const value_type& value, true_type);  // true_type means keys are unique.
        iterator DoInsertValue(const value_type& value, false_type);                    // false_type means keys are not unique.

        eastl::pair<iterator, bool> DoInsertKey(const key_type& key, true_type);
        iterator DoInsertKey(const key_type& key, false_type);

        iterator DoInsertValue(iterator position, const value_type& value, true_type);
        iterator DoInsertValue(iterator position, const value_type& value, false_type);

        iterator DoInsertKey(iterator position, const key_type& key, true_type);
        iterator DoInsertKey(iterator position, const key_type& key, false_type);

        iterator DoInsertValueImpl(node_type* pNodeParent, const value_type& value, bool bForceToLeft);
        iterator DoInsertKeyImpl(node_type* pNodeParent, const key_type& key, bool bForceToLeft);

    }; // rbtree





    ///////////////////////////////////////////////////////////////////////
    // rbtree_node_base functions
    ///////////////////////////////////////////////////////////////////////

    EASTL_API inline rbtree_node_base* RBTreeGetMinChild(const rbtree_node_base* pNodeBase)
    {
        while(pNodeBase->mpNodeLeft) 
            pNodeBase = pNodeBase->mpNodeLeft;
        return const_cast<rbtree_node_base*>(pNodeBase);
    }

    EASTL_API inline rbtree_node_base* RBTreeGetMaxChild(const rbtree_node_base* pNodeBase)
    {
        while(pNodeBase->mpNodeRight) 
            pNodeBase = pNodeBase->mpNodeRight;
        return const_cast<rbtree_node_base*>(pNodeBase);
    }

    // The rest of the functions are non-trivial and are found in 
    // the corresponding .cpp file to this file.



    ///////////////////////////////////////////////////////////////////////
    // rbtree_iterator functions
    ///////////////////////////////////////////////////////////////////////

    template <typename T, typename Pointer, typename Reference>
    rbtree_iterator<T, Pointer, Reference>::rbtree_iterator()
        : mpNode(NULL) { }


    template <typename T, typename Pointer, typename Reference>
    rbtree_iterator<T, Pointer, Reference>::rbtree_iterator(const node_type* pNode)
        : mpNode(static_cast<node_type*>(const_cast<node_type*>(pNode))) { }


    template <typename T, typename Pointer, typename Reference>
    rbtree_iterator<T, Pointer, Reference>::rbtree_iterator(const iterator& x)
        : mpNode(x.mpNode) { }


    template <typename T, typename Pointer, typename Reference>
    typename rbtree_iterator<T, Pointer, Reference>::reference
    rbtree_iterator<T, Pointer, Reference>::operator*() const
        { return mpNode->mValue; }


    template <typename T, typename Pointer, typename Reference>
    typename rbtree_iterator<T, Pointer, Reference>::pointer
    rbtree_iterator<T, Pointer, Reference>::operator->() const
        { return &mpNode->mValue; }


    template <typename T, typename Pointer, typename Reference>
    typename rbtree_iterator<T, Pointer, Reference>::this_type&
    rbtree_iterator<T, Pointer, Reference>::operator++()
    {
        mpNode = static_cast<node_type*>(RBTreeIncrement(mpNode));
        return *this;
    }


    template <typename T, typename Pointer, typename Reference>
    typename rbtree_iterator<T, Pointer, Reference>::this_type
    rbtree_iterator<T, Pointer, Reference>::operator++(int)
    {
        this_type temp(*this);
        mpNode = static_cast<node_type*>(RBTreeIncrement(mpNode));
        return temp;
    }


    template <typename T, typename Pointer, typename Reference>
    typename rbtree_iterator<T, Pointer, Reference>::this_type&
    rbtree_iterator<T, Pointer, Reference>::operator--()
    {
        mpNode = static_cast<node_type*>(RBTreeDecrement(mpNode));
        return *this;
    }


    template <typename T, typename Pointer, typename Reference>
    typename rbtree_iterator<T, Pointer, Reference>::this_type
    rbtree_iterator<T, Pointer, Reference>::operator--(int)
    {
        this_type temp(*this);
        mpNode = static_cast<node_type*>(RBTreeDecrement(mpNode));
        return temp;
    }


    // The C++ defect report #179 requires that we support comparisons between const and non-const iterators.
    // Thus we provide additional template paremeters here to support this. The defect report does not
    // require us to support comparisons between reverse_iterators and const_reverse_iterators.
    template <typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB>
    inline bool operator==(const rbtree_iterator<T, PointerA, ReferenceA>& a, 
                           const rbtree_iterator<T, PointerB, ReferenceB>& b)
    {
        return a.mpNode == b.mpNode;
    }


    template <typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB>
    inline bool operator!=(const rbtree_iterator<T, PointerA, ReferenceA>& a, 
                           const rbtree_iterator<T, PointerB, ReferenceB>& b)
    {
        return a.mpNode != b.mpNode;
    }


    // We provide a version of operator!= for the case where the iterators are of the 
    // same type. This helps prevent ambiguity errors in the presence of rel_ops.
    template <typename T, typename Pointer, typename Reference>
    inline bool operator!=(const rbtree_iterator<T, Pointer, Reference>& a, 
                           const rbtree_iterator<T, Pointer, Reference>& b)
    {
        return a.mpNode != b.mpNode;
    }




    ///////////////////////////////////////////////////////////////////////
    // rbtree functions
    ///////////////////////////////////////////////////////////////////////

    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline rbtree<K, V, C, A, E, bM, bU>::rbtree()
        : mAnchor(),
          mnSize(0),
          mAllocator(EASTL_RBTREE_DEFAULT_NAME)
    {
        reset();
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline rbtree<K, V, C, A, E, bM, bU>::rbtree(const allocator_type& allocator)
        : mAnchor(),
          mnSize(0),
          mAllocator(allocator)
    {
        reset();
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline rbtree<K, V, C, A, E, bM, bU>::rbtree(const C& compare, const allocator_type& allocator)
        : base_type(compare),
          mAnchor(),
          mnSize(0),
          mAllocator(allocator)
    {
        reset();
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline rbtree<K, V, C, A, E, bM, bU>::rbtree(const this_type& x)
        : base_type(x.mCompare),
          mAnchor(),
          mnSize(0),
          mAllocator(x.mAllocator)
    {
        reset();

        if(x.mAnchor.mpNodeParent) // mAnchor.mpNodeParent is the rb_tree root node.
        {
            mAnchor.mpNodeParent = DoCopySubtree((const node_type*)x.mAnchor.mpNodeParent, (node_type*)&mAnchor);
            mAnchor.mpNodeRight  = RBTreeGetMaxChild(mAnchor.mpNodeParent);
            mAnchor.mpNodeLeft   = RBTreeGetMinChild(mAnchor.mpNodeParent);
            mnSize               = x.mnSize;
        }
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    template <typename InputIterator>
    inline rbtree<K, V, C, A, E, bM, bU>::rbtree(InputIterator first, InputIterator last, const C& compare, const allocator_type& allocator)
        : base_type(compare),
          mAnchor(),
          mnSize(0),
          mAllocator(allocator)
    {
        reset();

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
                throw;
            }
        #endif
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline rbtree<K, V, C, A, E, bM, bU>::~rbtree()
    {
        // Erase the entire tree. DoNukeSubtree is not a 
        // conventional erase function, as it does no rebalancing.
        DoNukeSubtree((node_type*)mAnchor.mpNodeParent);
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline typename rbtree<K, V, C, A, E, bM, bU>::allocator_type&
    rbtree<K, V, C, A, E, bM, bU>::get_allocator()
    {
        return mAllocator;
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline void rbtree<K, V, C, A, E, bM, bU>::set_allocator(const allocator_type& allocator)
    {
        mAllocator = allocator;
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline typename rbtree<K, V, C, A, E, bM, bU>::size_type
    rbtree<K, V, C, A, E, bM, bU>::size() const
        { return mnSize; }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline bool rbtree<K, V, C, A, E, bM, bU>::empty() const
        { return (mnSize == 0); }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline typename rbtree<K, V, C, A, E, bM, bU>::iterator
    rbtree<K, V, C, A, E, bM, bU>::begin()
        { return iterator(static_cast<node_type*>(mAnchor.mpNodeLeft)); }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline typename rbtree<K, V, C, A, E, bM, bU>::const_iterator
    rbtree<K, V, C, A, E, bM, bU>::begin() const
        { return const_iterator(static_cast<node_type*>(const_cast<rbtree_node_base*>(mAnchor.mpNodeLeft))); }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline typename rbtree<K, V, C, A, E, bM, bU>::iterator
    rbtree<K, V, C, A, E, bM, bU>::end()
        { return iterator(static_cast<node_type*>(&mAnchor)); }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline typename rbtree<K, V, C, A, E, bM, bU>::const_iterator
    rbtree<K, V, C, A, E, bM, bU>::end() const
        { return const_iterator(static_cast<node_type*>(const_cast<rbtree_node_base*>(&mAnchor))); }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline typename rbtree<K, V, C, A, E, bM, bU>::reverse_iterator
    rbtree<K, V, C, A, E, bM, bU>::rbegin()
        { return reverse_iterator(end()); }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline typename rbtree<K, V, C, A, E, bM, bU>::const_reverse_iterator
    rbtree<K, V, C, A, E, bM, bU>::rbegin() const
        { return const_reverse_iterator(end()); }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline typename rbtree<K, V, C, A, E, bM, bU>::reverse_iterator
    rbtree<K, V, C, A, E, bM, bU>::rend()
        { return reverse_iterator(begin()); }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline typename rbtree<K, V, C, A, E, bM, bU>::const_reverse_iterator
    rbtree<K, V, C, A, E, bM, bU>::rend() const
        { return const_reverse_iterator(begin()); }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline typename rbtree<K, V, C, A, E, bM, bU>::this_type&
    rbtree<K, V, C, A, E, bM, bU>::operator=(const this_type& x)
    {
        if(this != &x)
        {
            clear();

            #if EASTL_ALLOCATOR_COPY_ENABLED
                mAllocator = x.mAllocator;
            #endif

            base_type::mCompare = x.mCompare;

            if(x.mAnchor.mpNodeParent) // mAnchor.mpNodeParent is the rb_tree root node.
            {
                mAnchor.mpNodeParent = DoCopySubtree((const node_type*)x.mAnchor.mpNodeParent, (node_type*)&mAnchor);
                mAnchor.mpNodeRight  = RBTreeGetMaxChild(mAnchor.mpNodeParent);
                mAnchor.mpNodeLeft   = RBTreeGetMinChild(mAnchor.mpNodeParent);
                mnSize               = x.mnSize;
            }
        }
        return *this;
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    void rbtree<K, V, C, A, E, bM, bU>::swap(this_type& x)
    {
        if(mAllocator == x.mAllocator) // If allocators are equivalent...
        {
            // Most of our members can be exchaged by a basic swap:
            // We leave mAllocator as-is.
            eastl::swap(mnSize,              x.mnSize);
            eastl::swap(base_type::mCompare, x.mCompare);

            // However, because our anchor node is a part of our class instance and not 
            // dynamically allocated, we can't do a swap of it but must do a more elaborate
            // procedure. This is the downside to having the mAnchor be like this, but 
            // otherwise we consider it a good idea to avoid allocating memory for a 
            // nominal container instance.

            // We optimize for the expected most common case: both pointers being non-null.
            if(mAnchor.mpNodeParent && x.mAnchor.mpNodeParent) // If both pointers are non-null...
            {
                eastl::swap(mAnchor.mpNodeRight,  x.mAnchor.mpNodeRight);
                eastl::swap(mAnchor.mpNodeLeft,   x.mAnchor.mpNodeLeft);
                eastl::swap(mAnchor.mpNodeParent, x.mAnchor.mpNodeParent);

                // We need to fix up the anchors to point to themselves (we can't just swap them).
                mAnchor.mpNodeParent->mpNodeParent   = &mAnchor;
                x.mAnchor.mpNodeParent->mpNodeParent = &x.mAnchor;
            }
            else if(mAnchor.mpNodeParent)
            {
                x.mAnchor.mpNodeRight  = mAnchor.mpNodeRight;
                x.mAnchor.mpNodeLeft   = mAnchor.mpNodeLeft;
                x.mAnchor.mpNodeParent = mAnchor.mpNodeParent;
                x.mAnchor.mpNodeParent->mpNodeParent = &x.mAnchor;

                // We need to fix up our anchor to point it itself (we can't have it swap with x).
                mAnchor.mpNodeRight  = &mAnchor;
                mAnchor.mpNodeLeft   = &mAnchor;
                mAnchor.mpNodeParent = NULL;
            }
            else if(x.mAnchor.mpNodeParent)
            {
                mAnchor.mpNodeRight  = x.mAnchor.mpNodeRight;
                mAnchor.mpNodeLeft   = x.mAnchor.mpNodeLeft;
                mAnchor.mpNodeParent = x.mAnchor.mpNodeParent;
                mAnchor.mpNodeParent->mpNodeParent = &mAnchor;

                // We need to fix up x's anchor to point it itself (we can't have it swap with us).
                x.mAnchor.mpNodeRight  = &x.mAnchor;
                x.mAnchor.mpNodeLeft   = &x.mAnchor;
                x.mAnchor.mpNodeParent = NULL;
            } // Else both are NULL and there is nothing to do.
        }
        else
        {
            const this_type temp(*this); // Can't call eastl::swap because that would
            *this = x;                   // itself call this member swap function.
            x     = temp;
        }
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline typename rbtree<K, V, C, A, E, bM, bU>::insert_return_type // map/set::insert return a pair, multimap/multiset::iterator return an iterator.
    rbtree<K, V, C, A, E, bM, bU>::insert(const value_type& value)
        { return DoInsertValue(value, has_unique_keys_type()); }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    typename rbtree<K, V, C, A, E, bM, bU>::iterator
    rbtree<K, V, C, A, E, bM, bU>::insert(iterator position, const value_type& value)
        { return DoInsertValue(position, value, has_unique_keys_type()); }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    eastl::pair<typename rbtree<K, V, C, A, E, bM, bU>::iterator, bool>
    rbtree<K, V, C, A, E, bM, bU>::DoInsertValue(const value_type& value, true_type) // true_type means keys are unique.
    {
        // This is the pathway for insertion of unique keys (map and set, but not multimap and multiset).
        // Note that we return a pair and not an iterator. This is because the C++ standard for map
        // and set is to return a pair and not just an iterator.
        extract_key extractKey;

        node_type* pCurrent    = (node_type*)mAnchor.mpNodeParent; // Start with the root node.
        node_type* pLowerBound = (node_type*)&mAnchor;             // Set it to the container end for now.
        node_type* pParent;                                        // This will be where we insert the new node.

        bool bValueLessThanNode = true; // If the tree is empty, this will result in an insertion at the front.

        // Find insertion position of the value. This will either be a position which 
        // already contains the value, a position which is greater than the value or
        // end(), which we treat like a position which is greater than the value.
        while(EASTL_LIKELY(pCurrent)) // Do a walk down the tree.
        {
            bValueLessThanNode = mCompare(extractKey(value), extractKey(pCurrent->mValue));
            pLowerBound        = pCurrent;

            if(bValueLessThanNode)
            {
                EASTL_VALIDATE_COMPARE(!mCompare(extractKey(pCurrent->mValue), extractKey(value))); // Validate that the compare function is sane.
                pCurrent = (node_type*)pCurrent->mpNodeLeft;
            }
            else
                pCurrent = (node_type*)pCurrent->mpNodeRight;
        }

        pParent = pLowerBound; // pLowerBound is actually upper bound right now (i.e. it is > value instead of <=), but we will make it the lower bound below.

        if(bValueLessThanNode) // If we ended up on the left side of the last parent node...
        {
            if(EASTL_LIKELY(pLowerBound != (node_type*)mAnchor.mpNodeLeft)) // If the tree was empty or if we otherwise need to insert at the very front of the tree...
            {
                // At this point, pLowerBound points to a node which is > than value.
                // Move it back by one, so that it points to a node which is <= value.
                pLowerBound = (node_type*)RBTreeDecrement(pLowerBound);
            }
            else
            {
                const iterator itResult(DoInsertValueImpl(pLowerBound, value, false));
                return pair<iterator, bool>(itResult, true);
            }
        }

        // Since here we require values to be unique, we will do nothing if the value already exists.
        if(mCompare(extractKey(pLowerBound->mValue), extractKey(value))) // If the node is < the value (i.e. if value is >= the node)...
        {
            EASTL_VALIDATE_COMPARE(!mCompare(extractKey(value), extractKey(pLowerBound->mValue))); // Validate that the compare function is sane.
            const iterator itResult(DoInsertValueImpl(pParent, value, false));
            return pair<iterator, bool>(itResult, true);
        }

        // The item already exists (as found by the compare directly above), so return false.
        return pair<iterator, bool>(iterator(pLowerBound), false);
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    typename rbtree<K, V, C, A, E, bM, bU>::iterator
    rbtree<K, V, C, A, E, bM, bU>::DoInsertValue(const value_type& value, false_type) // false_type means keys are not unique.
    {
        // This is the pathway for insertion of non-unique keys (multimap and multiset, but not map and set).
        node_type* pCurrent  = (node_type*)mAnchor.mpNodeParent; // Start with the root node.
        node_type* pRangeEnd = (node_type*)&mAnchor;             // Set it to the container end for now.
        extract_key extractKey;

        while(pCurrent)
        {
            pRangeEnd = pCurrent;

            if(mCompare(extractKey(value), extractKey(pCurrent->mValue)))
            {
                EASTL_VALIDATE_COMPARE(!mCompare(extractKey(pCurrent->mValue), extractKey(value))); // Validate that the compare function is sane.
                pCurrent = (node_type*)pCurrent->mpNodeLeft;
            }
            else
                pCurrent = (node_type*)pCurrent->mpNodeRight;
        }

        return DoInsertValueImpl(pRangeEnd, value, false);
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    eastl::pair<typename rbtree<K, V, C, A, E, bM, bU>::iterator, bool>
    rbtree<K, V, C, A, E, bM, bU>::DoInsertKey(const key_type& key, true_type) // true_type means keys are unique.
    {
        // This code is essentially a slightly modified copy of the the rbtree::insert 
        // function whereby this version takes a key and not a full value_type.
        extract_key extractKey;

        node_type* pCurrent    = (node_type*)mAnchor.mpNodeParent; // Start with the root node.
        node_type* pLowerBound = (node_type*)&mAnchor;             // Set it to the container end for now.
        node_type* pParent;                                        // This will be where we insert the new node.

        bool bValueLessThanNode = true; // If the tree is empty, this will result in an insertion at the front.

        // Find insertion position of the value. This will either be a position which 
        // already contains the value, a position which is greater than the value or
        // end(), which we treat like a position which is greater than the value.
        while(EASTL_LIKELY(pCurrent)) // Do a walk down the tree.
        {
            bValueLessThanNode = mCompare(key, extractKey(pCurrent->mValue));
            pLowerBound        = pCurrent;

            if(bValueLessThanNode)
            {
                EASTL_VALIDATE_COMPARE(!mCompare(extractKey(pCurrent->mValue), key)); // Validate that the compare function is sane.
                pCurrent = (node_type*)pCurrent->mpNodeLeft;
            }
            else
                pCurrent = (node_type*)pCurrent->mpNodeRight;
        }

        pParent = pLowerBound; // pLowerBound is actually upper bound right now (i.e. it is > value instead of <=), but we will make it the lower bound below.

        if(bValueLessThanNode) // If we ended up on the left side of the last parent node...
        {
            if(EASTL_LIKELY(pLowerBound != (node_type*)mAnchor.mpNodeLeft)) // If the tree was empty or if we otherwise need to insert at the very front of the tree...
            {
                // At this point, pLowerBound points to a node which is > than value.
                // Move it back by one, so that it points to a node which is <= value.
                pLowerBound = (node_type*)RBTreeDecrement(pLowerBound);
            }
            else
            {
                const iterator itResult(DoInsertKeyImpl(pLowerBound, key, false));
                return pair<iterator, bool>(itResult, true);
            }
        }

        // Since here we require values to be unique, we will do nothing if the value already exists.
        if(mCompare(extractKey(pLowerBound->mValue), key)) // If the node is < the value (i.e. if value is >= the node)...
        {
            EASTL_VALIDATE_COMPARE(!mCompare(key, extractKey(pLowerBound->mValue))); // Validate that the compare function is sane.
            const iterator itResult(DoInsertKeyImpl(pParent, key, false));
            return pair<iterator, bool>(itResult, true);
        }

        // The item already exists (as found by the compare directly above), so return false.
        return pair<iterator, bool>(iterator(pLowerBound), false);
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    typename rbtree<K, V, C, A, E, bM, bU>::iterator
    rbtree<K, V, C, A, E, bM, bU>::DoInsertKey(const key_type& key, false_type) // false_type means keys are not unique.
    {
        // This is the pathway for insertion of non-unique keys (multimap and multiset, but not map and set).
        node_type* pCurrent  = (node_type*)mAnchor.mpNodeParent; // Start with the root node.
        node_type* pRangeEnd = (node_type*)&mAnchor;             // Set it to the container end for now.
        extract_key extractKey;

        while(pCurrent)
        {
            pRangeEnd = pCurrent;

            if(mCompare(key, extractKey(pCurrent->mValue)))
            {
                EASTL_VALIDATE_COMPARE(!mCompare(extractKey(pCurrent->mValue), key)); // Validate that the compare function is sane.
                pCurrent = (node_type*)pCurrent->mpNodeLeft;
            }
            else
                pCurrent = (node_type*)pCurrent->mpNodeRight;
        }

        return DoInsertKeyImpl(pRangeEnd, key, false);
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    typename rbtree<K, V, C, A, E, bM, bU>::iterator
    rbtree<K, V, C, A, E, bM, bU>::DoInsertValue(iterator position, const value_type& value, true_type) // true_type means keys are unique.
    {
        // This is the pathway for insertion of unique keys (map and set, but not multimap and multiset).
        //
        // We follow the same approach as SGI STL/STLPort and use the position as
        // a forced insertion position for the value when possible.
        extract_key extractKey;

        if((position.mpNode != mAnchor.mpNodeRight) && (position.mpNode != &mAnchor)) // If the user specified a specific insertion position...
        {
            iterator itNext(position);
            ++itNext;

            // To consider: Change this so that 'position' specifies the position after 
            // where the insertion goes and not the position before where the insertion goes.
            // Doing so would make this more in line with user expectations and with LWG #233.
            const bool bPositionLessThanValue = mCompare(extractKey(position.mpNode->mValue), extractKey(value));

            if(bPositionLessThanValue) // If (value > *position)...
            {
                EASTL_VALIDATE_COMPARE(!mCompare(extractKey(value), extractKey(position.mpNode->mValue))); // Validate that the compare function is sane.

                const bool bValueLessThanNext = mCompare(extractKey(value), extractKey(itNext.mpNode->mValue));

                if(bValueLessThanNext) // if (value < *itNext)...
                {
                    EASTL_VALIDATE_COMPARE(!mCompare(extractKey(itNext.mpNode->mValue), extractKey(value))); // Validate that the compare function is sane.

                    if(position.mpNode->mpNodeRight)
                        return DoInsertValueImpl(itNext.mpNode, value, true);
                    return DoInsertValueImpl(position.mpNode, value, false);
                }
            }

            return DoInsertValue(value, has_unique_keys_type()).first;
        }

        if(mnSize && mCompare(extractKey(((node_type*)mAnchor.mpNodeRight)->mValue), extractKey(value)))
        {
            EASTL_VALIDATE_COMPARE(!mCompare(extractKey(value), extractKey(((node_type*)mAnchor.mpNodeRight)->mValue))); // Validate that the compare function is sane.
            return DoInsertValueImpl((node_type*)mAnchor.mpNodeRight, value, false);
        }

        return DoInsertValue(value, has_unique_keys_type()).first;
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    typename rbtree<K, V, C, A, E, bM, bU>::iterator
    rbtree<K, V, C, A, E, bM, bU>::DoInsertValue(iterator position, const value_type& value, false_type) // false_type means keys are not unique.
    {
        // This is the pathway for insertion of non-unique keys (multimap and multiset, but not map and set).
        //
        // We follow the same approach as SGI STL/STLPort and use the position as
        // a forced insertion position for the value when possible.
        extract_key extractKey;

        if((position.mpNode != mAnchor.mpNodeRight) && (position.mpNode != &mAnchor)) // If the user specified a specific insertion position...
        {
            iterator itNext(position);
            ++itNext;

            // To consider: Change this so that 'position' specifies the position after 
            // where the insertion goes and not the position before where the insertion goes.
            // Doing so would make this more in line with user expectations and with LWG #233.

            if(!mCompare(extractKey(value), extractKey(position.mpNode->mValue)) && // If value >= *position && 
               !mCompare(extractKey(itNext.mpNode->mValue), extractKey(value)))     // if value <= *itNext...
            {
                if(position.mpNode->mpNodeRight) // If there are any nodes to the right... [this expression will always be true as long as we aren't at the end()]
                    return DoInsertValueImpl(itNext.mpNode, value, true); // Specifically insert in front of (to the left of) itNext (and thus after 'position').
                return DoInsertValueImpl(position.mpNode, value, false);
            }

            return DoInsertValue(value, has_unique_keys_type()); // If the above specified hint was not useful, then we do a regular insertion.
        }

        // This pathway shouldn't be commonly executed, as the user shouldn't be calling 
        // this hinted version of insert if the user isn't providing a useful hint.

        if(mnSize && !mCompare(extractKey(value), extractKey(((node_type*)mAnchor.mpNodeRight)->mValue))) // If we are non-empty and the value is >= the last node...
            return DoInsertValueImpl((node_type*)mAnchor.mpNodeRight, value, false); // Insert after the last node (doesn't matter if we force left or not).

        return DoInsertValue(value, has_unique_keys_type()); // We are empty or we are inserting at the end.
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    typename rbtree<K, V, C, A, E, bM, bU>::iterator
    rbtree<K, V, C, A, E, bM, bU>::DoInsertKey(iterator position, const key_type& key, true_type) // true_type means keys are unique.
    {
        // This is the pathway for insertion of unique keys (map and set, but not multimap and multiset).
        //
        // We follow the same approach as SGI STL/STLPort and use the position as
        // a forced insertion position for the value when possible.
        extract_key extractKey;

        if((position.mpNode != mAnchor.mpNodeRight) && (position.mpNode != &mAnchor)) // If the user specified a specific insertion position...
        {
            iterator itNext(position);
            ++itNext;

            // To consider: Change this so that 'position' specifies the position after 
            // where the insertion goes and not the position before where the insertion goes.
            // Doing so would make this more in line with user expectations and with LWG #233.
            const bool bPositionLessThanValue = mCompare(extractKey(position.mpNode->mValue), key);

            if(bPositionLessThanValue) // If (value > *position)...
            {
                EASTL_VALIDATE_COMPARE(!mCompare(key, extractKey(position.mpNode->mValue))); // Validate that the compare function is sane.

                const bool bValueLessThanNext = mCompare(key, extractKey(itNext.mpNode->mValue));

                if(bValueLessThanNext) // If value < *itNext...
                {
                    EASTL_VALIDATE_COMPARE(!mCompare(extractKey(itNext.mpNode->mValue), key)); // Validate that the compare function is sane.

                    if(position.mpNode->mpNodeRight)
                        return DoInsertKeyImpl(itNext.mpNode, key, true);
                    return DoInsertKeyImpl(position.mpNode, key, false);
                }
            }

            return DoInsertKey(key, has_unique_keys_type()).first;
        }

        if(mnSize && mCompare(extractKey(((node_type*)mAnchor.mpNodeRight)->mValue), key))
        {
            EASTL_VALIDATE_COMPARE(!mCompare(key, extractKey(((node_type*)mAnchor.mpNodeRight)->mValue))); // Validate that the compare function is sane.
            return DoInsertKeyImpl((node_type*)mAnchor.mpNodeRight, key, false);
        }

        return DoInsertKey(key, has_unique_keys_type()).first;
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    typename rbtree<K, V, C, A, E, bM, bU>::iterator
    rbtree<K, V, C, A, E, bM, bU>::DoInsertKey(iterator position, const key_type& key, false_type) // false_type means keys are not unique.
    {
        // This is the pathway for insertion of non-unique keys (multimap and multiset, but not map and set).
        //
        // We follow the same approach as SGI STL/STLPort and use the position as
        // a forced insertion position for the value when possible.
        extract_key extractKey;

        if((position.mpNode != mAnchor.mpNodeRight) && (position.mpNode != &mAnchor)) // If the user specified a specific insertion position...
        {
            iterator itNext(position);
            ++itNext;

            // To consider: Change this so that 'position' specifies the position after 
            // where the insertion goes and not the position before where the insertion goes.
            // Doing so would make this more in line with user expectations and with LWG #233.
            if(!mCompare(key, extractKey(position.mpNode->mValue)) && // If value >= *position && 
               !mCompare(extractKey(itNext.mpNode->mValue), key))     // if value <= *itNext...
            {
                if(position.mpNode->mpNodeRight) // If there are any nodes to the right... [this expression will always be true as long as we aren't at the end()]
                    return DoInsertKeyImpl(itNext.mpNode, key, true); // Specifically insert in front of (to the left of) itNext (and thus after 'position').
                return DoInsertKeyImpl(position.mpNode, key, false);
            }

            return DoInsertKey(key, has_unique_keys_type()); // If the above specified hint was not useful, then we do a regular insertion.
        }

        // This pathway shouldn't be commonly executed, as the user shouldn't be calling 
        // this hinted version of insert if the user isn't providing a useful hint.
        if(mnSize && !mCompare(key, extractKey(((node_type*)mAnchor.mpNodeRight)->mValue))) // If we are non-empty and the value is >= the last node...
            return DoInsertKeyImpl((node_type*)mAnchor.mpNodeRight, key, false); // Insert after the last node (doesn't matter if we force left or not).

        return DoInsertKey(key, has_unique_keys_type()); // We are empty or we are inserting at the end.
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    typename rbtree<K, V, C, A, E, bM, bU>::iterator
    rbtree<K, V, C, A, E, bM, bU>::DoInsertValueImpl(node_type* pNodeParent, const value_type& value, bool bForceToLeft)
    {
        RBTreeSide side;
        extract_key extractKey;

        // The reason we may want to have bForceToLeft == true is that pNodeParent->mValue and value may be equal.
        // In that case it doesn't matter what side we insert on, except that the C++ LWG #233 improvement report
        // suggests that we should use the insert hint position to force an ordering. So that's what we do.
        if(bForceToLeft || (pNodeParent == &mAnchor) || mCompare(extractKey(value), extractKey(pNodeParent->mValue)))
            side = kRBTreeSideLeft;
        else
            side = kRBTreeSideRight;

        node_type* const pNodeNew = DoCreateNode(value); // Note that pNodeNew->mpLeft, mpRight, mpParent, will be uninitialized.
        RBTreeInsert(pNodeNew, pNodeParent, &mAnchor, side);
        mnSize++;

        return iterator(pNodeNew);
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    typename rbtree<K, V, C, A, E, bM, bU>::iterator
    rbtree<K, V, C, A, E, bM, bU>::DoInsertKeyImpl(node_type* pNodeParent, const key_type& key, bool bForceToLeft)
    {
        RBTreeSide side;
        extract_key extractKey;

        // The reason we may want to have bForceToLeft == true is that pNodeParent->mValue and value may be equal.
        // In that case it doesn't matter what side we insert on, except that the C++ LWG #233 improvement report
        // suggests that we should use the insert hint position to force an ordering. So that's what we do.
        if(bForceToLeft || (pNodeParent == &mAnchor) || mCompare(key, extractKey(pNodeParent->mValue)))
            side = kRBTreeSideLeft;
        else
            side = kRBTreeSideRight;

        node_type* const pNodeNew = DoCreateNodeFromKey(key); // Note that pNodeNew->mpLeft, mpRight, mpParent, will be uninitialized.
        RBTreeInsert(pNodeNew, pNodeParent, &mAnchor, side);
        mnSize++;

        return iterator(pNodeNew);
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    template <typename InputIterator>
    void rbtree<K, V, C, A, E, bM, bU>::insert(InputIterator first, InputIterator last)
    {
        for( ; first != last; ++first)
            DoInsertValue(*first, has_unique_keys_type()); // Or maybe we should call 'insert(end(), *first)' instead. If the first-last range was sorted then this might make some sense.
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline void rbtree<K, V, C, A, E, bM, bU>::clear()
    {
        // Erase the entire tree. DoNukeSubtree is not a 
        // conventional erase function, as it does no rebalancing.
        DoNukeSubtree((node_type*)mAnchor.mpNodeParent);
        reset();
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline void rbtree<K, V, C, A, E, bM, bU>::reset()
    {
        // The reset function is a special extension function which unilaterally 
        // resets the container to an empty state without freeing the memory of 
        // the contained objects. This is useful for very quickly tearing down a 
        // container built into scratch memory.
        mAnchor.mpNodeRight  = &mAnchor;
        mAnchor.mpNodeLeft   = &mAnchor;
        mAnchor.mpNodeParent = NULL;
        mAnchor.mColor       = kRBTreeColorRed;
        mnSize               = 0;
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline typename rbtree<K, V, C, A, E, bM, bU>::iterator
    rbtree<K, V, C, A, E, bM, bU>::erase(iterator position)
    {
        const iterator iErase(position);
        --mnSize; // Interleave this between the two references to itNext. We expect no exceptions to occur during the code below.
        ++position;
        RBTreeErase(iErase.mpNode, &mAnchor);
        DoFreeNode(iErase.mpNode);
        return position;
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    typename rbtree<K, V, C, A, E, bM, bU>::iterator
    rbtree<K, V, C, A, E, bM, bU>::erase(iterator first, iterator last)
    {
        // We expect that if the user means to clear the container, they will call clear.
        if(EASTL_LIKELY((first.mpNode != mAnchor.mpNodeLeft) || (last.mpNode != &mAnchor))) // If (first != begin or last != end) ...
        {
            // Basic implementation:
            while(first != last)
                first = erase(first);
            return first;

            // Inlined implementation:
            //size_type n = 0;
            //while(first != last)
            //{
            //    const iterator itErase(first);
            //    ++n;
            //    ++first;
            //    RBTreeErase(itErase.mpNode, &mAnchor);
            //    DoFreeNode(itErase.mpNode);
            //}
            //mnSize -= n;
            //return first;
        }

        clear();
        return iterator((node_type*)&mAnchor); // Same as: return end();
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline typename rbtree<K, V, C, A, E, bM, bU>::reverse_iterator
    rbtree<K, V, C, A, E, bM, bU>::erase(reverse_iterator position)
    {
        return reverse_iterator(erase((++position).base()));
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    typename rbtree<K, V, C, A, E, bM, bU>::reverse_iterator
    rbtree<K, V, C, A, E, bM, bU>::erase(reverse_iterator first, reverse_iterator last)
    {
        // Version which erases in order from first to last.
        // difference_type i(first.base() - last.base());
        // while(i--)
        //     first = erase(first);
        // return first;

        // Version which erases in order from last to first, but is slightly more efficient:
        return reverse_iterator(erase((++last).base(), (++first).base()));
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline void rbtree<K, V, C, A, E, bM, bU>::erase(const key_type* first, const key_type* last)
    {
        // We have no choice but to run a loop like this, as the first/last range could
        // have values that are discontiguously located in the tree. And some may not 
        // even be in the tree.
        while(first != last)
            erase(*first++);
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    typename rbtree<K, V, C, A, E, bM, bU>::iterator
    rbtree<K, V, C, A, E, bM, bU>::find(const key_type& key)
    {
        // To consider: Implement this instead via calling lower_bound and 
        // inspecting the result. The following is an implementation of this:
        //    const iterator it(lower_bound(key));
        //    return ((it.mpNode == &mAnchor) || mCompare(key, extractKey(it.mpNode->mValue))) ? iterator(&mAnchor) : it;
        // We don't currently implement the above because in practice people tend to call 
        // find a lot with trees, but very uncommonly call lower_bound.
        extract_key extractKey;

        node_type* pCurrent  = (node_type*)mAnchor.mpNodeParent; // Start with the root node.
        node_type* pRangeEnd = (node_type*)&mAnchor;             // Set it to the container end for now.

        while(EASTL_LIKELY(pCurrent)) // Do a walk down the tree.
        {
            if(EASTL_LIKELY(!mCompare(extractKey(pCurrent->mValue), key))) // If pCurrent is >= key...
            {
                pRangeEnd = pCurrent;
                pCurrent  = (node_type*)pCurrent->mpNodeLeft;
            }
            else
            {
                EASTL_VALIDATE_COMPARE(!mCompare(key, extractKey(pCurrent->mValue))); // Validate that the compare function is sane.
                pCurrent  = (node_type*)pCurrent->mpNodeRight;
            }
        }

        if(EASTL_LIKELY((pRangeEnd != &mAnchor) && !mCompare(key, extractKey(pRangeEnd->mValue))))
            return iterator(pRangeEnd);
        return iterator((node_type*)&mAnchor);
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline typename rbtree<K, V, C, A, E, bM, bU>::const_iterator
    rbtree<K, V, C, A, E, bM, bU>::find(const key_type& key) const
    {
        typedef rbtree<K, V, C, A, E, bM, bU> rbtree_type;
        return const_iterator(const_cast<rbtree_type*>(this)->find(key));
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    template <typename U, typename Compare2>
    typename rbtree<K, V, C, A, E, bM, bU>::iterator
    rbtree<K, V, C, A, E, bM, bU>::find_as(const U& u, Compare2 compare2)
    {
        extract_key extractKey;

        node_type* pCurrent  = (node_type*)mAnchor.mpNodeParent; // Start with the root node.
        node_type* pRangeEnd = (node_type*)&mAnchor;             // Set it to the container end for now.

        while(EASTL_LIKELY(pCurrent)) // Do a walk down the tree.
        {
            if(EASTL_LIKELY(!compare2(extractKey(pCurrent->mValue), u))) // If pCurrent is >= u...
            {
                pRangeEnd = pCurrent;
                pCurrent  = (node_type*)pCurrent->mpNodeLeft;
            }
            else
            {
                EASTL_VALIDATE_COMPARE(!compare2(u, extractKey(pCurrent->mValue))); // Validate that the compare function is sane.
                pCurrent  = (node_type*)pCurrent->mpNodeRight;
            }
        }

        if(EASTL_LIKELY((pRangeEnd != &mAnchor) && !compare2(u, extractKey(pRangeEnd->mValue))))
            return iterator(pRangeEnd);
        return iterator((node_type*)&mAnchor);
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    template <typename U, typename Compare2>
    inline typename rbtree<K, V, C, A, E, bM, bU>::const_iterator
    rbtree<K, V, C, A, E, bM, bU>::find_as(const U& u, Compare2 compare2) const
    {
        typedef rbtree<K, V, C, A, E, bM, bU> rbtree_type;
        return const_iterator(const_cast<rbtree_type*>(this)->find_as(u, compare2));
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    typename rbtree<K, V, C, A, E, bM, bU>::iterator
    rbtree<K, V, C, A, E, bM, bU>::lower_bound(const key_type& key)
    {
        extract_key extractKey;

        node_type* pCurrent  = (node_type*)mAnchor.mpNodeParent; // Start with the root node.
        node_type* pRangeEnd = (node_type*)&mAnchor;             // Set it to the container end for now.

        while(EASTL_LIKELY(pCurrent)) // Do a walk down the tree.
        {
            if(EASTL_LIKELY(!mCompare(extractKey(pCurrent->mValue), key))) // If pCurrent is >= key...
            {
                pRangeEnd = pCurrent;
                pCurrent  = (node_type*)pCurrent->mpNodeLeft;
            }
            else
            {
                EASTL_VALIDATE_COMPARE(!mCompare(key, extractKey(pCurrent->mValue))); // Validate that the compare function is sane.
                pCurrent  = (node_type*)pCurrent->mpNodeRight;
            }
        }

        return iterator(pRangeEnd);
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline typename rbtree<K, V, C, A, E, bM, bU>::const_iterator
    rbtree<K, V, C, A, E, bM, bU>::lower_bound(const key_type& key) const
    {
        typedef rbtree<K, V, C, A, E, bM, bU> rbtree_type;
        return const_iterator(const_cast<rbtree_type*>(this)->lower_bound(key));
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    typename rbtree<K, V, C, A, E, bM, bU>::iterator
    rbtree<K, V, C, A, E, bM, bU>::upper_bound(const key_type& key)
    {
        extract_key extractKey;

        node_type* pCurrent  = (node_type*)mAnchor.mpNodeParent; // Start with the root node.
        node_type* pRangeEnd = (node_type*)&mAnchor;             // Set it to the container end for now.

        while(EASTL_LIKELY(pCurrent)) // Do a walk down the tree.
        {
            if(EASTL_LIKELY(mCompare(key, extractKey(pCurrent->mValue)))) // If key is < pCurrent...
            {
                EASTL_VALIDATE_COMPARE(!mCompare(extractKey(pCurrent->mValue), key)); // Validate that the compare function is sane.
                pRangeEnd = pCurrent;
                pCurrent  = (node_type*)pCurrent->mpNodeLeft;
            }
            else
                pCurrent  = (node_type*)pCurrent->mpNodeRight;
        }

        return iterator(pRangeEnd);
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline typename rbtree<K, V, C, A, E, bM, bU>::const_iterator
    rbtree<K, V, C, A, E, bM, bU>::upper_bound(const key_type& key) const
    {
        typedef rbtree<K, V, C, A, E, bM, bU> rbtree_type;
        return const_iterator(const_cast<rbtree_type*>(this)->upper_bound(key));
    }


    // To do: Move this validate function entirely to a template-less implementation.
    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    bool rbtree<K, V, C, A, E, bM, bU>::validate() const
    {
        // Red-black trees have the following canonical properties which we validate here:
        //   1 Every node is either red or black.
        //   2 Every leaf (NULL) is black by defintion. Any number of black nodes may appear in a sequence. 
        //   3 If a node is red, then both its children are black. Thus, on any path from 
        //     the root to a leaf, red nodes must not be adjacent.
        //   4 Every simple path from a node to a descendant leaf contains the same number of black nodes.
        //   5 The mnSize member of the tree must equal the number of nodes in the tree.
        //   6 The tree is sorted as per a conventional binary tree.
        //   7 The comparison function is sane; it obeys strict weak ordering. If mCompare(a,b) is true, then mCompare(b,a) must be false. Both cannot be true.

        extract_key extractKey;

        if(mnSize)
        {
            // Verify basic integrity.
            //if(!mAnchor.mpNodeParent || (mAnchor.mpNodeLeft == mAnchor.mpNodeRight))
            //    return false;             // Fix this for case of empty tree.

            if(mAnchor.mpNodeLeft != RBTreeGetMinChild(mAnchor.mpNodeParent))
                return false;

            if(mAnchor.mpNodeRight != RBTreeGetMaxChild(mAnchor.mpNodeParent))
                return false;

            const size_t nBlackCount   = RBTreeGetBlackCount(mAnchor.mpNodeParent, mAnchor.mpNodeLeft);
            size_type    nIteratedSize = 0;

            for(const_iterator it = begin(); it != end(); ++it, ++nIteratedSize)
            {
                const node_type* const pNode      = (const node_type*)it.mpNode;
                const node_type* const pNodeRight = (const node_type*)pNode->mpNodeRight;
                const node_type* const pNodeLeft  = (const node_type*)pNode->mpNodeLeft;

                // Verify #7 above.
                if(pNodeRight && mCompare(extractKey(pNodeRight->mValue), extractKey(pNode->mValue)) && mCompare(extractKey(pNode->mValue), extractKey(pNodeRight->mValue))) // Validate that the compare function is sane.
                    return false;

                // Verify #7 above.
                if(pNodeLeft && mCompare(extractKey(pNodeLeft->mValue), extractKey(pNode->mValue)) && mCompare(extractKey(pNode->mValue), extractKey(pNodeLeft->mValue))) // Validate that the compare function is sane.
                    return false;

                // Verify item #1 above.
                if((pNode->mColor != kRBTreeColorRed) && (pNode->mColor != kRBTreeColorBlack))
                    return false;

                // Verify item #3 above.
                if(pNode->mColor == kRBTreeColorRed)
                {
                    if((pNodeRight && (pNodeRight->mColor == kRBTreeColorRed)) ||
                       (pNodeLeft  && (pNodeLeft->mColor  == kRBTreeColorRed)))
                        return false;
                }

                // Verify item #6 above.
                if(pNodeRight && mCompare(extractKey(pNodeRight->mValue), extractKey(pNode->mValue)))
                    return false;

                if(pNodeLeft && mCompare(extractKey(pNode->mValue), extractKey(pNodeLeft->mValue)))
                    return false;

                if(!pNodeRight && !pNodeLeft) // If we are at a bottom node of the tree...
                {
                    // Verify item #4 above.
                    if(RBTreeGetBlackCount(mAnchor.mpNodeParent, pNode) != nBlackCount)
                        return false;
                }
            }

            // Verify item #5 above.
            if(nIteratedSize != mnSize)
                return false;

            return true;
        }
        else
        {
            if((mAnchor.mpNodeLeft != &mAnchor) || (mAnchor.mpNodeRight != &mAnchor))
                return false;
        }

        return true;
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline int rbtree<K, V, C, A, E, bM, bU>::validate_iterator(const_iterator i) const
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


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline typename rbtree<K, V, C, A, E, bM, bU>::node_type*
    rbtree<K, V, C, A, E, bM, bU>::DoAllocateNode()
    {
        return (node_type*)allocate_memory(mAllocator, sizeof(node_type), kValueAlignment, kValueAlignmentOffset);
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    inline void rbtree<K, V, C, A, E, bM, bU>::DoFreeNode(node_type* pNode)
    {
        pNode->~node_type();
        EASTLFree(mAllocator, pNode, sizeof(node_type));
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    typename rbtree<K, V, C, A, E, bM, bU>::node_type*
    rbtree<K, V, C, A, E, bM, bU>::DoCreateNodeFromKey(const key_type& key)
    {
        // Note that this function intentionally leaves the node pointers uninitialized.
        // The caller would otherwise just turn right around and modify them, so there's
        // no point in us initializing them to anything (except in a debug build).
        node_type* const pNode = DoAllocateNode();

        #if EASTL_EXCEPTIONS_ENABLED
            try
            {
        #endif
                ::new(&pNode->mValue) value_type(key);

        #if EASTL_EXCEPTIONS_ENABLED
            }
            catch(...)
            {
                DoFreeNode(pNode);
                throw;
            }
        #endif

        #if EASTL_DEBUG
            pNode->mpNodeRight  = NULL;
            pNode->mpNodeLeft   = NULL;
            pNode->mpNodeParent = NULL;
            pNode->mColor       = kRBTreeColorBlack;
        #endif

        return pNode;
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    typename rbtree<K, V, C, A, E, bM, bU>::node_type*
    rbtree<K, V, C, A, E, bM, bU>::DoCreateNode(const value_type& value)
    {
        // Note that this function intentionally leaves the node pointers uninitialized.
        // The caller would otherwise just turn right around and modify them, so there's
        // no point in us initializing them to anything (except in a debug build).
        node_type* const pNode = DoAllocateNode();

        #if EASTL_EXCEPTIONS_ENABLED
            try
            {
        #endif
                ::new(&pNode->mValue) value_type(value);
        #if EASTL_EXCEPTIONS_ENABLED
            }
            catch(...)
            {
                DoFreeNode(pNode);
                throw;
            }
        #endif

        #if EASTL_DEBUG
            pNode->mpNodeRight  = NULL;
            pNode->mpNodeLeft   = NULL;
            pNode->mpNodeParent = NULL;
            pNode->mColor       = kRBTreeColorBlack;
        #endif

        return pNode;
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    typename rbtree<K, V, C, A, E, bM, bU>::node_type*
    rbtree<K, V, C, A, E, bM, bU>::DoCreateNode(const node_type* pNodeSource, node_type* pNodeParent)
    {
        node_type* const pNode = DoCreateNode(pNodeSource->mValue);

        pNode->mpNodeRight  = NULL;
        pNode->mpNodeLeft   = NULL;
        pNode->mpNodeParent = pNodeParent;
        pNode->mColor       = pNodeSource->mColor;

        return pNode;
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    typename rbtree<K, V, C, A, E, bM, bU>::node_type*
    rbtree<K, V, C, A, E, bM, bU>::DoCopySubtree(const node_type* pNodeSource, node_type* pNodeDest)
    {
        node_type* const pNewNodeRoot = DoCreateNode(pNodeSource, pNodeDest);

        #if EASTL_EXCEPTIONS_ENABLED
            try
            {
        #endif
                // Copy the right side of the tree recursively.
                if(pNodeSource->mpNodeRight)
                    pNewNodeRoot->mpNodeRight = DoCopySubtree((const node_type*)pNodeSource->mpNodeRight, pNewNodeRoot);

                node_type* pNewNodeLeft;

                for(pNodeSource = (node_type*)pNodeSource->mpNodeLeft, pNodeDest = pNewNodeRoot; 
                    pNodeSource;
                    pNodeSource = (node_type*)pNodeSource->mpNodeLeft, pNodeDest = pNewNodeLeft)
                {
                    pNewNodeLeft = DoCreateNode(pNodeSource, pNodeDest);

                    pNodeDest->mpNodeLeft = pNewNodeLeft;

                    // Copy the right side of the tree recursively.
                    if(pNodeSource->mpNodeRight)
                        pNewNodeLeft->mpNodeRight = DoCopySubtree((const node_type*)pNodeSource->mpNodeRight, pNewNodeLeft);
                }
        #if EASTL_EXCEPTIONS_ENABLED
            }
            catch(...)
            {
                DoNukeSubtree(pNewNodeRoot);
                throw;
            }
        #endif

        return pNewNodeRoot;
    }


    template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
    void rbtree<K, V, C, A, E, bM, bU>::DoNukeSubtree(node_type* pNode)
    {
        while(pNode) // Recursively traverse the tree and destroy items as we go.
        {
            DoNukeSubtree((node_type*)pNode->mpNodeRight);

            node_type* const pNodeLeft = (node_type*)pNode->mpNodeLeft;
            DoFreeNode(pNode);
            pNode = pNodeLeft;
        }
    }



    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template <typename K, typename V, typename A, typename C, typename E, bool bM, bool bU>
    inline bool operator==(const rbtree<K, V, C, A, E, bM, bU>& a, const rbtree<K, V, C, A, E, bM, bU>& b)
    {
        return (a.size() == b.size()) && eastl::equal(a.begin(), a.end(), b.begin());
    }


    // Note that in operator< we do comparisons based on the tree value_type with operator<() of the
    // value_type instead of the tree's Compare function. For set/multiset, the value_type is T, while
    // for map/multimap the value_type is a pair<Key, T>. operator< for pair can be seen by looking
    // utility.h, but it basically is uses the operator< for pair.first and pair.second. The C++ standard
    // appears to require this behaviour, whether intentionally or not. If anything, a good reason to do
    // this is for consistency. A map and a vector that contain the same items should compare the same.
    template <typename K, typename V, typename A, typename C, typename E, bool bM, bool bU>
    inline bool operator<(const rbtree<K, V, C, A, E, bM, bU>& a, const rbtree<K, V, C, A, E, bM, bU>& b)
    {
        return eastl::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
    }


    template <typename K, typename V, typename A, typename C, typename E, bool bM, bool bU>
    inline bool operator!=(const rbtree<K, V, C, A, E, bM, bU>& a, const rbtree<K, V, C, A, E, bM, bU>& b)
    {
        return !(a == b);
    }


    template <typename K, typename V, typename A, typename C, typename E, bool bM, bool bU>
    inline bool operator>(const rbtree<K, V, C, A, E, bM, bU>& a, const rbtree<K, V, C, A, E, bM, bU>& b)
    {
        return b < a;
    }


    template <typename K, typename V, typename A, typename C, typename E, bool bM, bool bU>
    inline bool operator<=(const rbtree<K, V, C, A, E, bM, bU>& a, const rbtree<K, V, C, A, E, bM, bU>& b)
    {
        return !(b < a);
    }


    template <typename K, typename V, typename A, typename C, typename E, bool bM, bool bU>
    inline bool operator>=(const rbtree<K, V, C, A, E, bM, bU>& a, const rbtree<K, V, C, A, E, bM, bU>& b)
    {
        return !(a < b);
    }


    template <typename K, typename V, typename A, typename C, typename E, bool bM, bool bU>
    inline void swap(rbtree<K, V, C, A, E, bM, bU>& a, rbtree<K, V, C, A, E, bM, bU>& b)
    {
        a.swap(b);
    }


} // namespace eastl


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard













