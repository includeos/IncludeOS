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
// EASTL/list.h
//
// Copyright (c) 2005, Electronic Arts. All rights reserved.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements a doubly-linked list, much like the C++ std::list class.
// The primary distinctions between this list and std::list are:
//    - list doesn't implement some of the less-frequently used functions 
//      of std::list. Any required functions can be added at a later time.
//    - list has a couple extension functions that increase performance.
//    - list can contain objects with alignment requirements. std::list cannot
//      do so without a bit of tedious non-portable effort.
//    - list has optimizations that don't exist in the STL implementations 
//      supplied by library vendors for our targeted platforms.
//    - list supports debug memory naming natively.
//    - list::size() by default is not a constant time function, like the list::size 
//      in some std implementations such as STLPort and SGI STL but unlike the 
//      list in Dinkumware and Metrowerks. The EASTL_LIST_SIZE_CACHE option can change this.
//    - list provides a guaranteed portable node definition that allows users
//      to write custom fixed size node allocators that are portable.
//    - list is easier to read, debug, and visualize.
//    - list is savvy to an environment that doesn't have exception handling,
//      as is sometimes the case with console or embedded environments.
//    - list has less deeply nested function calls and allows the user to 
//      enable forced inlining in debug builds in order to reduce bloat.
//    - list doesn't keep a member size variable. This means that list is 
//      smaller than std::list (depends on std::list) and that for most operations
//      it is faster than std::list. However, the list::size function is slower.
//    - list::size_type is defined as eastl_size_t instead of size_t in order to 
//      save memory and run faster on 64 bit systems.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_LIST_H
#define EASTL_LIST_H


#include <EASTL/internal/config.h>
#include <EASTL/allocator.h>
#include <EASTL/type_traits.h>
#include <EASTL/iterator.h>
#include <EASTL/algorithm.h>

#ifdef _MSC_VER
    #pragma warning(push, 0)
    #include <new>
    #include <stddef.h>
    #pragma warning(pop)
#else
    #include <new>
    #include <stddef.h>
#endif

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4530)  // C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
    #pragma warning(disable: 4345)  // Behavior change: an object of POD type constructed with an initializer of the form () will be default-initialized
#endif


namespace eastl
{

    /// EASTL_LIST_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    ///
    #ifndef EASTL_LIST_DEFAULT_NAME
        #define EASTL_LIST_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " list" // Unless the user overrides something, this is "EASTL list".
    #endif


    /// EASTL_LIST_DEFAULT_ALLOCATOR
    ///
    #ifndef EASTL_LIST_DEFAULT_ALLOCATOR
        #define EASTL_LIST_DEFAULT_ALLOCATOR allocator_type(EASTL_LIST_DEFAULT_NAME)
    #endif



    /// ListNodeBase
    ///
    /// We define a ListNodeBase separately from ListNode (below), because it allows
    /// us to have non-templated operations such as insert, remove (below), and it 
    /// makes it so that the list anchor node doesn't carry a T with it, which would
    /// waste space and possibly lead to surprising the user due to extra Ts existing
    /// that the user didn't explicitly create. The downside to all of this is that 
    /// it makes debug viewing of a list harder, given that the node pointers are of 
    /// type ListNodeBase and not ListNode. However, see ListNodeBaseProxy below.
    ///
    struct ListNodeBase
    {
        ListNodeBase* mpNext;
        ListNodeBase* mpPrev;

        void        insert(ListNodeBase* pNext);
        void        remove();
        void        splice(ListNodeBase* pFirst, ListNodeBase* pLast);
        void        reverse();
        static void swap(ListNodeBase& a, ListNodeBase& b);
    } EASTL_LIST_PROXY_MAY_ALIAS;


    #if EASTL_LIST_PROXY_ENABLED

        /// ListNodeBaseProxy
        ///
        /// In debug builds, we define ListNodeBaseProxy to be the same thing as
        /// ListNodeBase, except it is templated on the parent ListNode class.
        /// We do this because we want users in debug builds to be able to easily
        /// view the list's contents in a debugger GUI. We do this only in a debug
        /// build for the reasons described above: that ListNodeBase needs to be
        /// as efficient as possible and not cause code bloat or extra function 
        /// calls (inlined or not).
        ///
        /// ListNodeBaseProxy *must* be separate from its parent class ListNode 
        /// because the list class must have a member node which contains no T value.
        /// It is thus incorrect for us to have one single ListNode class which
        /// has mpNext, mpPrev, and mValue. So we do a recursive template trick in 
        /// the definition and use of SListNodeBaseProxy.
        ///
        template <typename LN>
        struct ListNodeBaseProxy
        {
            LN* mpNext;
            LN* mpPrev;
        };

        template <typename T>
        struct ListNode : public ListNodeBaseProxy< ListNode<T> >
        {
            T mValue;
        };

    #else

        template <typename T>
        struct ListNode : public ListNodeBase
        {
            T mValue;
        };

    #endif




    /// ListIterator
    ///
    template <typename T, typename Pointer, typename Reference>
    struct ListIterator
    {
        typedef ListIterator<T, Pointer, Reference>         this_type;
        typedef ListIterator<T, T*, T&>                     iterator;
        typedef ListIterator<T, const T*, const T&>         const_iterator;
        typedef eastl_size_t                                size_type;     // See config.h for the definition of eastl_size_t, which defaults to uint32_t.
        typedef ptrdiff_t                                   difference_type;
        typedef T                                           value_type;
        typedef ListNode<T>                                 node_type;
        typedef Pointer                                     pointer;
        typedef Reference                                   reference;
        typedef EASTL_ITC_NS::bidirectional_iterator_tag    iterator_category;

    public:
        node_type* mpNode;

    public:
        ListIterator();
        ListIterator(const ListNodeBase* pNode);
        ListIterator(const iterator& x);

        reference operator*() const;
        pointer   operator->() const;

        this_type& operator++();
        this_type  operator++(int);

        this_type& operator--();
        this_type  operator--(int);

    }; // ListIterator




    /// ListBase
    ///
    /// See VectorBase (class vector) for an explanation of why we 
    /// create this separate base class.
    ///
    template <typename T, typename Allocator>
    class ListBase
    {
    public:
        typedef T                                    value_type;
        typedef Allocator                            allocator_type;
        typedef ListNode<T>                          node_type;
        typedef eastl_size_t                         size_type;     // See config.h for the definition of eastl_size_t, which defaults to uint32_t.
        typedef ptrdiff_t                            difference_type;
        #if EASTL_LIST_PROXY_ENABLED
            typedef ListNodeBaseProxy< ListNode<T> > base_node_type;
        #else
            typedef ListNodeBase                     base_node_type; // We use ListNodeBase instead of ListNode<T> because we don't want to create a T.
        #endif

        enum
        {
            kAlignment       = EASTL_ALIGN_OF(T),
            kAlignmentOffset = 0                    // offsetof(node_type, mValue);
        };

    protected:
        base_node_type mNode;
        #if EASTL_LIST_SIZE_CACHE
            size_type  mSize;
        #endif
        allocator_type mAllocator;  // To do: Use base class optimization to make this go away.

    public:
        allocator_type& get_allocator();
        void            set_allocator(const allocator_type& allocator);

    protected:
        ListBase();
        ListBase(const allocator_type& a);
       ~ListBase();

        node_type* DoAllocateNode();
        void       DoFreeNode(node_type* pNode);

        void DoInit();
        void DoClear();

    }; // ListBase




    /// list
    ///
    /// -- size() is O(n) --
    /// Note that as of this writing, list::size() is an O(n) operation. That is, getting the size
    /// of the list is not a fast operation, as it requires traversing the list and counting the nodes.
    /// We could make list::size() be fast by having a member mSize variable. There are reasons for 
    /// having such functionality and reasons for not having such functionality. We currently choose
    /// to not have a member mSize variable as it would add four bytes to the class, add a tiny amount
    /// of processing to functions such as insert and erase, and would only serve to improve the size
    /// function, but no others. The alternative argument is that the C++ standard states that std::list
    /// should be an O(1) operation (i.e. have a member size variable), most C++ standard library list
    /// implementations do so, the size is but an integer which is quick to update, and many users 
    /// expect to have a fast size function. The EASTL_LIST_SIZE_CACHE option changes this.
    /// To consider: Make size caching an optional template parameter.
    ///
    /// Pool allocation
    /// If you want to make a custom memory pool for a list container, your pool 
    /// needs to contain items of type list::node_type. So if you have a memory
    /// pool that has a constructor that takes the size of pool items and the
    /// count of pool items, you would do this (assuming that MemoryPool implements
    /// the Allocator interface):
    ///     typedef list<Widget, MemoryPool> WidgetList;           // Delare your WidgetList type.
    ///     MemoryPool myPool(sizeof(WidgetList::node_type), 100); // Make a pool of 100 Widget nodes.
    ///     WidgetList myList(&myPool);                            // Create a list that uses the pool.
    ///
    template <typename T, typename Allocator = EASTLAllocatorType>
    class list : public ListBase<T, Allocator>
    {
        typedef ListBase<T, Allocator>                  base_type;
        typedef list<T, Allocator>                      this_type;

    public:
        typedef T                                       value_type;
        typedef T*                                      pointer;
        typedef const T*                                const_pointer;
        typedef T&                                      reference;
        typedef const T&                                const_reference;
        typedef ListIterator<T, T*, T&>                 iterator;
        typedef ListIterator<T, const T*, const T&>     const_iterator;
        typedef eastl::reverse_iterator<iterator>       reverse_iterator;
        typedef eastl::reverse_iterator<const_iterator> const_reverse_iterator;
        typedef typename base_type::size_type           size_type;
        typedef typename base_type::difference_type     difference_type;
        typedef typename base_type::allocator_type      allocator_type;
        typedef typename base_type::node_type           node_type;
        typedef typename base_type::base_node_type      base_node_type;

        using base_type::mNode;
        using base_type::mAllocator;
        using base_type::DoAllocateNode;
        using base_type::DoFreeNode;
        using base_type::DoClear;
        using base_type::DoInit;
        using base_type::get_allocator;
        #if EASTL_LIST_SIZE_CACHE
            using base_type::mSize;
        #endif

    public:
        list();
        list(const allocator_type& allocator);
        explicit list(size_type n, const allocator_type& allocator = EASTL_LIST_DEFAULT_ALLOCATOR);
        list(size_type n, const value_type& value, const allocator_type& allocator = EASTL_LIST_DEFAULT_ALLOCATOR);
        list(const this_type& x);

        template <typename InputIterator>
        list(InputIterator first, InputIterator last); // allocator arg removed because VC7.1 fails on the default arg. To do: Make a second version of this function without a default arg.

        this_type& operator=(const this_type& x);
        void swap(this_type& x);

        void assign(size_type n, const value_type& value);

        template <typename InputIterator>                       // It turns out that the C++ std::list specifies a two argument
        void assign(InputIterator first, InputIterator last);   // version of assign that takes (int size, int value). These are not 
                                                                // iterators, so we need to do a template compiler trick to do the right thing.
        iterator       begin();
        const_iterator begin() const;
        iterator       end();
        const_iterator end() const;

        reverse_iterator       rbegin();
        const_reverse_iterator rbegin() const;
        reverse_iterator       rend();
        const_reverse_iterator rend() const;

        bool      empty() const;
        size_type size() const;

        void resize(size_type n, const value_type& value);
        void resize(size_type n);

        reference       front();
        const_reference front() const;

        reference       back();
        const_reference back() const;

        void      push_front(const value_type& value);
        reference push_front();
        void*     push_front_uninitialized();

        void      push_back(const value_type& value);
        reference push_back();
        void*     push_back_uninitialized();

        void pop_front();
        void pop_back();

        iterator insert(iterator position);
        iterator insert(iterator position, const value_type& value);

        void insert(iterator position, size_type n, const value_type& value);

        template <typename InputIterator>
        void insert(iterator position, InputIterator first, InputIterator last);

        iterator erase(iterator position);
        iterator erase(iterator first, iterator last);

        reverse_iterator erase(reverse_iterator position);
        reverse_iterator erase(reverse_iterator first, reverse_iterator last);

        void clear();
        void reset();

        void remove(const T& x);

        template <typename Predicate>
        void remove_if(Predicate);

        void reverse();

        void splice(iterator position, this_type& x);
        void splice(iterator position, this_type& x, iterator i);
        void splice(iterator position, this_type& x, iterator first, iterator last);

    public:
        // Sorting functionality
        // This is independent of the global sort algorithms, as lists are 
        // linked nodes and can be sorted more efficiently by moving nodes
        // around in ways that global sort algorithms aren't privy to.

        void merge(this_type& x);

        template <typename Compare>
        void merge(this_type& x, Compare compare);

        void unique();

        template <typename BinaryPredicate>
        void unique(BinaryPredicate);

        void sort();

        template<typename Compare>
        void sort(Compare compare);

    public:
        bool validate() const;
        int  validate_iterator(const_iterator i) const;

    protected:
        node_type* DoCreateNode();
        node_type* DoCreateNode(const value_type& value);

        template <typename Integer>
        void DoAssign(Integer n, Integer value, true_type);

        template <typename InputIterator>
        void DoAssign(InputIterator first, InputIterator last, false_type);

        void DoAssignValues(size_type n, const value_type& value);

        template <typename Integer>
        void DoInsert(ListNodeBase* pNode, Integer n, Integer value, true_type);

        template <typename InputIterator>
        void DoInsert(ListNodeBase* pNode, InputIterator first, InputIterator last, false_type);

        void DoInsertValues(ListNodeBase* pNode, size_type n, const value_type& value);

        void DoInsertValue(ListNodeBase* pNode, const value_type& value);

        void DoErase(ListNodeBase* pNode);

    }; // class list





    ///////////////////////////////////////////////////////////////////////
    // ListNodeBase
    ///////////////////////////////////////////////////////////////////////

    inline void ListNodeBase::swap(ListNodeBase& a, ListNodeBase& b)
    {
        const ListNodeBase temp(a);
        a = b;
        b = temp;

        if(a.mpNext == &b)
            a.mpNext = a.mpPrev = &a;
        else
            a.mpNext->mpPrev = a.mpPrev->mpNext = &a;

        if(b.mpNext == &a)
            b.mpNext = b.mpPrev = &b;
        else
            b.mpNext->mpPrev = b.mpPrev->mpNext = &b;
    }


    inline void ListNodeBase::splice(ListNodeBase* first, ListNodeBase* last)
    {
        // We assume that [first, last] are not within our list.
        last->mpPrev->mpNext  = this;
        first->mpPrev->mpNext = last;
        this->mpPrev->mpNext  = first;

        ListNodeBase* const pTemp = this->mpPrev;
        this->mpPrev  = last->mpPrev;
        last->mpPrev  = first->mpPrev;
        first->mpPrev = pTemp;
    }


    inline void ListNodeBase::reverse()
    {
        ListNodeBase* pNode = this;
        do
        {
            ListNodeBase* const pTemp = pNode->mpNext;
            pNode->mpNext = pNode->mpPrev;
            pNode->mpPrev = pTemp;
            pNode         = pNode->mpPrev;
        } 
        while(pNode != this);
    }


    inline void ListNodeBase::insert(ListNodeBase* pNext)
    {
        mpNext = pNext;
        mpPrev = pNext->mpPrev;
        pNext->mpPrev->mpNext = this;
        pNext->mpPrev = this;
    }


    inline void ListNodeBase::remove()
    {
        mpPrev->mpNext = mpNext;
        mpNext->mpPrev = mpPrev;
    }




    ///////////////////////////////////////////////////////////////////////
    // ListIterator
    ///////////////////////////////////////////////////////////////////////

    template <typename T, typename Pointer, typename Reference>
    inline ListIterator<T, Pointer, Reference>::ListIterator()
        : mpNode() // To consider: Do we really need to intialize mpNode?
    {
        // Empty
    }


    template <typename T, typename Pointer, typename Reference>
    inline ListIterator<T, Pointer, Reference>::ListIterator(const ListNodeBase* pNode)
        : mpNode(static_cast<node_type*>((ListNode<T>*)const_cast<ListNodeBase*>(pNode))) // All this casting is in the name of making runtime debugging much easier on the user.
    {
        // Empty
    }


    template <typename T, typename Pointer, typename Reference>
    inline ListIterator<T, Pointer, Reference>::ListIterator(const iterator& x)
        : mpNode(const_cast<node_type*>(x.mpNode))
    {
        // Empty
    } 


    template <typename T, typename Pointer, typename Reference>
    inline typename ListIterator<T, Pointer, Reference>::reference
    ListIterator<T, Pointer, Reference>::operator*() const
    {
        return mpNode->mValue;
    }


    template <typename T, typename Pointer, typename Reference>
    inline typename ListIterator<T, Pointer, Reference>::pointer
    ListIterator<T, Pointer, Reference>::operator->() const
    {
        return &mpNode->mValue;
    }


    template <typename T, typename Pointer, typename Reference>
    inline typename ListIterator<T, Pointer, Reference>::this_type&
    ListIterator<T, Pointer, Reference>::operator++()
    {
        mpNode = static_cast<node_type*>(mpNode->mpNext);
        return *this;
    }


    template <typename T, typename Pointer, typename Reference>
    inline typename ListIterator<T, Pointer, Reference>::this_type
    ListIterator<T, Pointer, Reference>::operator++(int)
    {
        this_type temp(*this);
        mpNode = static_cast<node_type*>(mpNode->mpNext);
        return temp;
    }


    template <typename T, typename Pointer, typename Reference>
    inline typename ListIterator<T, Pointer, Reference>::this_type&
    ListIterator<T, Pointer, Reference>::operator--()
    {
        mpNode = static_cast<node_type*>(mpNode->mpPrev);
        return *this;
    }


    template <typename T, typename Pointer, typename Reference>
    inline typename ListIterator<T, Pointer, Reference>::this_type 
    ListIterator<T, Pointer, Reference>::operator--(int)
    {
        this_type temp(*this);
        mpNode = static_cast<node_type*>(mpNode->mpPrev);
        return temp;
    }


    // The C++ defect report #179 requires that we support comparisons between const and non-const iterators.
    // Thus we provide additional template paremeters here to support this. The defect report does not
    // require us to support comparisons between reverse_iterators and const_reverse_iterators.
    template <typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB>
    inline bool operator==(const ListIterator<T, PointerA, ReferenceA>& a, 
                           const ListIterator<T, PointerB, ReferenceB>& b)
    {
        return a.mpNode == b.mpNode;
    }


    template <typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB>
    inline bool operator!=(const ListIterator<T, PointerA, ReferenceA>& a, 
                           const ListIterator<T, PointerB, ReferenceB>& b)
    {
        return a.mpNode != b.mpNode;
    }


    // We provide a version of operator!= for the case where the iterators are of the 
    // same type. This helps prevent ambiguity errors in the presence of rel_ops.
    template <typename T, typename Pointer, typename Reference>
    inline bool operator!=(const ListIterator<T, Pointer, Reference>& a, 
                           const ListIterator<T, Pointer, Reference>& b)
    {
        return a.mpNode != b.mpNode;
    }



    ///////////////////////////////////////////////////////////////////////
    // ListBase
    ///////////////////////////////////////////////////////////////////////

    template <typename T, typename Allocator>
    inline ListBase<T, Allocator>::ListBase()
        : mNode(),
          #if EASTL_LIST_SIZE_CACHE
          mSize(0),
          #endif
          mAllocator(EASTL_LIST_DEFAULT_NAME)
    {
        DoInit();
    }

    template <typename T, typename Allocator>
    inline ListBase<T, Allocator>::ListBase(const allocator_type& allocator)
        : mNode(),
          #if EASTL_LIST_SIZE_CACHE
          mSize(0),
          #endif
          mAllocator(allocator)
    {
        DoInit();
    }


    template <typename T, typename Allocator>
    inline ListBase<T, Allocator>::~ListBase()
    {
        DoClear();
    }


    template <typename T, typename Allocator>
    typename ListBase<T, Allocator>::allocator_type&
    ListBase<T, Allocator>::get_allocator()
    {
        return mAllocator;
    }


    template <typename T, typename Allocator>
    inline void ListBase<T, Allocator>::set_allocator(const allocator_type& allocator)
    {
        mAllocator = allocator;
    }


    template <typename T, typename Allocator>
    inline typename ListBase<T, Allocator>::node_type*
    ListBase<T, Allocator>::DoAllocateNode()
    {
        return (node_type*)allocate_memory(mAllocator, sizeof(node_type), kAlignment, kAlignmentOffset);
    }


    template <typename T, typename Allocator>
    inline void ListBase<T, Allocator>::DoFreeNode(node_type* p)
    {
        EASTLFree(mAllocator, p, sizeof(node_type));
    }


    template <typename T, typename Allocator>
    inline void ListBase<T, Allocator>::DoInit()
    {
        mNode.mpNext = (ListNode<T>*)&mNode;
        mNode.mpPrev = (ListNode<T>*)&mNode;
    }


    template <typename T, typename Allocator>
    inline void ListBase<T, Allocator>::DoClear()
    {
        node_type* p = static_cast<node_type*>(mNode.mpNext);

        while(p != &mNode)
        {
            node_type* const pTemp = p;
            p = static_cast<node_type*>(p->mpNext);
            pTemp->~node_type();
            EASTLFree(mAllocator, pTemp, sizeof(node_type));
        }
    }





    ///////////////////////////////////////////////////////////////////////
    // list
    ///////////////////////////////////////////////////////////////////////

    template <typename T, typename Allocator>
    inline list<T, Allocator>::list()
        : base_type()
    {
        // Empty
    }


    template <typename T, typename Allocator>
    inline list<T, Allocator>::list(const allocator_type& allocator)
        : base_type(allocator)
    {
        // Empty
    }


    template <typename T, typename Allocator>
    inline list<T, Allocator>::list(size_type n, const allocator_type& allocator)
        : base_type(allocator)
    {
        //insert(iterator((ListNodeBase*)&mNode), n, value_type());
        DoInsertValues((ListNodeBase*)&mNode, n, value_type());
    }


    template <typename T, typename Allocator>
    inline list<T, Allocator>::list(size_type n, const value_type& value, const allocator_type& allocator)
        : base_type(allocator) 
    {
        // insert(iterator((ListNodeBase*)&mNode), n, value);
        DoInsertValues((ListNodeBase*)&mNode, n, value);
    }


    template <typename T, typename Allocator>
    inline list<T, Allocator>::list(const this_type& x)
        : base_type(x.mAllocator)
    {
        //insert(iterator((ListNodeBase*)&mNode), const_iterator((ListNodeBase*)x.mNode.mpNext), const_iterator((ListNodeBase*)&x.mNode));
        DoInsert((ListNodeBase*)&mNode, const_iterator((const ListNodeBase*)x.mNode.mpNext), const_iterator((const ListNodeBase*)&x.mNode), false_type());
    }


    template <typename T, typename Allocator>
    template <typename InputIterator>
    list<T, Allocator>::list(InputIterator first, InputIterator last)
        : base_type(EASTL_LIST_DEFAULT_ALLOCATOR)
    {
        //insert(iterator((ListNodeBase*)&mNode), first, last);
        DoInsert((ListNodeBase*)&mNode, first, last, is_integral<InputIterator>());
    }


    template <typename T, typename Allocator>
    typename list<T, Allocator>::iterator
    inline list<T, Allocator>::begin()
    {
        return iterator((ListNodeBase*)mNode.mpNext);
    }


    template <typename T, typename Allocator>
    inline typename list<T, Allocator>::const_iterator
    list<T, Allocator>::begin() const
    {
        return const_iterator((ListNodeBase*)mNode.mpNext);
    }


    template <typename T, typename Allocator>
    inline typename list<T, Allocator>::iterator
    list<T, Allocator>::end()
    {
        return iterator((ListNodeBase*)&mNode);
    }


    template <typename T, typename Allocator>
    inline typename list<T, Allocator>::const_iterator
    list<T, Allocator>::end() const
    {
        return const_iterator((const ListNodeBase*)&mNode);
    }


    template <typename T, typename Allocator>
    inline typename list<T, Allocator>::reverse_iterator
    list<T, Allocator>::rbegin()
    {
        return reverse_iterator((ListNodeBase*)&mNode);
    }


    template <typename T, typename Allocator>
    inline typename list<T, Allocator>::const_reverse_iterator
    list<T, Allocator>::rbegin() const
    {
        return const_reverse_iterator((const ListNodeBase*)&mNode);
    }


    template <typename T, typename Allocator>
    inline typename list<T, Allocator>::reverse_iterator
    list<T, Allocator>::rend()
    {
        return reverse_iterator((ListNodeBase*)mNode.mpNext);
    }


    template <typename T, typename Allocator>
    inline typename list<T, Allocator>::const_reverse_iterator
    list<T, Allocator>::rend() const
    {
        return const_reverse_iterator((ListNodeBase*)mNode.mpNext);
    }


    template <typename T, typename Allocator>
    inline typename list<T, Allocator>::reference
    list<T, Allocator>::front()
    {
        #if EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
            // We allow the user to reference an empty container.
        #elif EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(static_cast<node_type*>(mNode.mpNext) == &mNode))
                EASTL_FAIL_MSG("list::front -- empty container");
        #endif

        return static_cast<node_type*>(mNode.mpNext)->mValue;
    }


    template <typename T, typename Allocator>
    inline typename list<T, Allocator>::const_reference
    list<T, Allocator>::front() const
    {
        #if EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
            // We allow the user to reference an empty container.
        #elif EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(static_cast<node_type*>(mNode.mpNext) == &mNode))
                EASTL_FAIL_MSG("list::front -- empty container");
        #endif

        return static_cast<node_type*>(mNode.mpNext)->mValue;
    }


    template <typename T, typename Allocator>
    inline typename list<T, Allocator>::reference
    list<T, Allocator>::back()
    {
        #if EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
            // We allow the user to reference an empty container.
        #elif EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(static_cast<node_type*>(mNode.mpNext) == &mNode))
                EASTL_FAIL_MSG("list::back -- empty container");
        #endif

        return static_cast<node_type*>(mNode.mpPrev)->mValue;
    }


    template <typename T, typename Allocator>
    inline typename list<T, Allocator>::const_reference
    list<T, Allocator>::back() const
    {
        #if EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
            // We allow the user to reference an empty container.
        #elif EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(static_cast<node_type*>(mNode.mpNext) == &mNode))
                EASTL_FAIL_MSG("list::back -- empty container");
        #endif

        return static_cast<node_type*>(mNode.mpPrev)->mValue;
    }


    template <typename T, typename Allocator>
    inline bool list<T, Allocator>::empty() const
    {
        return static_cast<node_type*>(mNode.mpNext) == &mNode;
    }


    template <typename T, typename Allocator>
    inline typename list<T, Allocator>::size_type
    list<T, Allocator>::size() const
    {
        #if EASTL_LIST_SIZE_CACHE
            return mSize;
        #else
            #if EASTL_DEBUG
                const ListNodeBase* p = (ListNodeBase*)mNode.mpNext;
                size_type n = 0;
                while(p != (const ListNodeBase*)&mNode)
                {
                    ++n;
                    p = (const ListNodeBase*)p->mpNext;
                }
                return n;
            #else
                // The following optimizes to slightly better code than the code above.
                return (size_type)eastl::distance(const_iterator((const ListNodeBase*)mNode.mpNext), const_iterator((const ListNodeBase*)&mNode));
            #endif
        #endif
    }


    template <typename T, typename Allocator>
    typename list<T, Allocator>::this_type&
    list<T, Allocator>::operator=(const this_type& x)
    {
        if(this != &x)
        {
            #if EASTL_ALLOCATOR_COPY_ENABLED
                mAllocator = x.mAllocator;
            #endif

            iterator       current((ListNodeBase*)mNode.mpNext);
            const_iterator first((const ListNodeBase*)x.mNode.mpNext);
            const_iterator last((const ListNodeBase*)&x.mNode);

            while((current.mpNode != &mNode) && (first != last))
            {
                *current = *first;
                ++first;
                ++current;
            }

            if(first == last)
                erase(current, (ListNodeBase*)&mNode);
            else
                insert((ListNodeBase*)&mNode, first, last);
        }
        return *this;
    }


    template <typename T, typename Allocator>
    inline void list<T, Allocator>::assign(size_type n, const value_type& value)
    {
        DoAssignValues(n, value);
    }


    // It turns out that the C++ std::list specifies a two argument
    // version of assign that takes (int size, int value). These are not 
    // iterators, so we need to do a template compiler trick to do the right thing.
    template <typename T, typename Allocator>
    template <typename InputIterator>
    inline void list<T, Allocator>::assign(InputIterator first, InputIterator last)
    {
        DoAssign(first, last, is_integral<InputIterator>());
    }


    template <typename T, typename Allocator>
    inline void list<T, Allocator>::clear()
    {
        DoClear();
        DoInit();
        #if EASTL_LIST_SIZE_CACHE
            mSize = 0;
        #endif
    }


    template <typename T, typename Allocator>
    inline void list<T, Allocator>::reset()
    {
        // The reset function is a special extension function which unilaterally 
        // resets the container to an empty state without freeing the memory of 
        // the contained objects. This is useful for very quickly tearing down a 
        // container built into scratch memory.
        DoInit();
        #if EASTL_LIST_SIZE_CACHE
            mSize = 0;
        #endif
    }


    template <typename T, typename Allocator>
    void list<T, Allocator>::resize(size_type n, const value_type& value)
    {
        iterator current((ListNodeBase*)mNode.mpNext);
        size_type i = 0;

        while((current.mpNode != &mNode) && (i < n))
        {
            ++current;  
            ++i;
        }
        if(i == n)
            erase(current, (ListNodeBase*)&mNode);
        else
            insert((ListNodeBase*)&mNode, n - i, value);
    }


    template <typename T, typename Allocator>
    inline void list<T, Allocator>::resize(size_type n)
    {
        resize(n, value_type());
    }


    template <typename T, typename Allocator>
    inline void list<T, Allocator>::push_front(const value_type& value)
    {
        DoInsertValue((ListNodeBase*)mNode.mpNext, value);
    }


    template <typename T, typename Allocator>
    inline typename list<T, Allocator>::reference
    list<T, Allocator>::push_front()
    {
        node_type* const pNode = DoCreateNode();
        ((ListNodeBase*)pNode)->insert((ListNodeBase*)mNode.mpNext);
        #if EASTL_LIST_SIZE_CACHE
            ++mSize;
        #endif
        return static_cast<node_type*>(mNode.mpNext)->mValue; // Same as return front();
    }


    template <typename T, typename Allocator>
    inline void* list<T, Allocator>::push_front_uninitialized()
    {
        node_type* const pNode = DoAllocateNode();
        ((ListNodeBase*)pNode)->insert((ListNodeBase*)mNode.mpNext);
        #if EASTL_LIST_SIZE_CACHE
            ++mSize;
        #endif
        return &pNode->mValue;
    }


    template <typename T, typename Allocator>
    inline void list<T, Allocator>::pop_front()
    {
        #if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(static_cast<node_type*>(mNode.mpNext) == &mNode))
                EASTL_FAIL_MSG("list::pop_front -- empty container");
        #endif

        DoErase((ListNodeBase*)mNode.mpNext);
    }


    template <typename T, typename Allocator>
    inline void list<T, Allocator>::push_back(const value_type& value)
    {
        DoInsertValue((ListNodeBase*)&mNode, value);
    }


    template <typename T, typename Allocator>
    inline typename list<T, Allocator>::reference
    list<T, Allocator>::push_back()
    {
        node_type* const pNode = DoCreateNode();
        ((ListNodeBase*)pNode)->insert((ListNodeBase*)&mNode);
        #if EASTL_LIST_SIZE_CACHE
            ++mSize;
        #endif
        return static_cast<node_type*>(mNode.mpPrev)->mValue;  // Same as return back();
    }


    template <typename T, typename Allocator>
    inline void* list<T, Allocator>::push_back_uninitialized()
    {
        node_type* const pNode = DoAllocateNode();
        ((ListNodeBase*)pNode)->insert((ListNodeBase*)&mNode);
        #if EASTL_LIST_SIZE_CACHE
            ++mSize;
        #endif
        return &pNode->mValue;
    }


    template <typename T, typename Allocator>
    inline void list<T, Allocator>::pop_back()
    {
        #if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(static_cast<node_type*>(mNode.mpNext) == &mNode))
                EASTL_FAIL_MSG("list::pop_back -- empty container");
        #endif

        DoErase((ListNodeBase*)mNode.mpPrev);
    }


    template <typename T, typename Allocator>
    inline typename list<T, Allocator>::iterator
    list<T, Allocator>::insert(iterator position)
    {
        node_type* const pNode = DoCreateNode(value_type());
        ((ListNodeBase*)pNode)->insert((ListNodeBase*)position.mpNode);
        #if EASTL_LIST_SIZE_CACHE
            ++mSize;
        #endif
        return (ListNodeBase*)pNode;
    }

    
    template <typename T, typename Allocator>
    inline typename list<T, Allocator>::iterator
    list<T, Allocator>::insert(iterator position, const value_type& value)
    {
        node_type* const pNode = DoCreateNode(value);
        ((ListNodeBase*)pNode)->insert((ListNodeBase*)position.mpNode);
        #if EASTL_LIST_SIZE_CACHE
            ++mSize;
        #endif
        return (ListNodeBase*)pNode;
    }


    template <typename T, typename Allocator>
    inline void list<T, Allocator>::insert(iterator position, size_type n, const value_type& value)
    {
        // To do: Get rid of DoInsertValues and put its implementation directly here.
        DoInsertValues((ListNodeBase*)position.mpNode, n, value);
    }


    template <typename T, typename Allocator>
    template <typename InputIterator>
    inline void list<T, Allocator>::insert(iterator position, InputIterator first, InputIterator last)
    {
        DoInsert((ListNodeBase*)position.mpNode, first, last, is_integral<InputIterator>());
    }


    template <typename T, typename Allocator>
    inline typename list<T, Allocator>::iterator
    list<T, Allocator>::erase(iterator position)
    {
        ++position;
        DoErase((ListNodeBase*)position.mpNode->mpPrev);
        return position;
    }


    template <typename T, typename Allocator>
    typename list<T, Allocator>::iterator
    list<T, Allocator>::erase(iterator first, iterator last)
    {
        while(first != last)
            first = erase(first);
        return last;
    }


    template <typename T, typename Allocator>
    inline typename list<T, Allocator>::reverse_iterator
    list<T, Allocator>::erase(reverse_iterator position)
    {
        return reverse_iterator(erase((++position).base()));
    }


    template <typename T, typename Allocator>
    typename list<T, Allocator>::reverse_iterator
    list<T, Allocator>::erase(reverse_iterator first, reverse_iterator last)
    {
        // Version which erases in order from first to last.
        // difference_type i(first.base() - last.base());
        // while(i--)
        //     first = erase(first);
        // return first;

        // Version which erases in order from last to first, but is slightly more efficient:
        return reverse_iterator(erase((++last).base(), (++first).base()));
    }


    template <typename T, typename Allocator>
    void list<T, Allocator>::remove(const value_type& value)
    {
        iterator current((ListNodeBase*)mNode.mpNext);

        while(current.mpNode != &mNode)
        {
            if(EASTL_LIKELY(!(*current == value)))
                ++current; // We have duplicate '++current' statements here and below, but the logic here forces this.
            else
            {
                ++current;
                DoErase((ListNodeBase*)current.mpNode->mpPrev);
            }
        }
    }


    template <typename T, typename Allocator>
    template <typename Predicate>
    inline void list<T, Allocator>::remove_if(Predicate predicate)
    {
        for(iterator first((ListNodeBase*)mNode.mpNext), last((ListNodeBase*)&mNode); first != last; )
        {
            iterator temp(first);
            ++temp;
            if(predicate(first.mpNode->mValue))
                DoErase((ListNodeBase*)first.mpNode);
            first = temp;
        }
    }


    template <typename T, typename Allocator>
    inline void list<T, Allocator>::reverse()
    {
        ((ListNodeBase&)mNode).reverse();
    }


    template <typename T, typename Allocator>
    inline void list<T, Allocator>::splice(iterator position, this_type& x)
    {
        // Splicing operations cannot succeed if the two containers use unequal allocators.
        // This issue is not addressed in the C++ 1998 standard but is discussed in the 
        // LWG defect reports, such as #431. There is no simple solution to this problem.
        // One option is to throw an exception. For now our answer is simply: don't do this.
        // EASTL_ASSERT(mAllocator == x.mAllocator); // Disabled because our member sort function uses splice but with allocators that may be unequal. There isn't a simple workaround aside from disabling this assert.

        // Disabled until the performance hit of this code is deemed worthwhile and until we are sure we want to disallow unequal allocators.
        //#if EASTL_EXCEPTIONS_ENABLED
        //    if(EASTL_UNLIKELY(!(mAllocator == x.mAllocator)))
        //        throw std::runtime_error("list::splice -- unequal allocators");
        //#endif

        #if EASTL_LIST_SIZE_CACHE
            if(x.mSize)
            {
                ((ListNodeBase*)position.mpNode)->splice((ListNodeBase*)x.mNode.mpNext, (ListNodeBase*)&x.mNode);
                mSize += x.mSize;
                x.mSize = 0;
            }
        #else
            if(!x.empty())
                ((ListNodeBase*)position.mpNode)->splice((ListNodeBase*)x.mNode.mpNext, (ListNodeBase*)&x.mNode);
        #endif
    }


    template <typename T, typename Allocator>
    inline void list<T, Allocator>::splice(iterator position, list& x, iterator i)
    {
        (void)x; // Avoid potential unused variable warnings.

        // See notes in the other splice function regarding this assertion.
        // EASTL_ASSERT(mAllocator == x.mAllocator); // Disabled because our member sort function uses splice but with allocators that may be unequal. There isn't a simple workaround aside from disabling this assert.

        // Disabled until the performance hit of this code is deemed worthwhile and until we are sure we want to disallow unequal allocators.
        //#if EASTL_EXCEPTIONS_ENABLED
        //    if(EASTL_UNLIKELY(!(mAllocator == x.mAllocator)))
        //        throw std::runtime_error("list::splice -- unequal allocators");
        //#endif

        iterator i2(i);
        ++i2;
        if((position != i) && (position != i2))
        {
            ((ListNodeBase*)position.mpNode)->splice((ListNodeBase*)i.mpNode, (ListNodeBase*)i2.mpNode);

            #if EASTL_LIST_SIZE_CACHE
                ++mSize;
                --x.mSize;
            #endif
        }
    }


    template <typename T, typename Allocator>
    inline void list<T, Allocator>::splice(iterator position, this_type& x, iterator first, iterator last)
    {
        (void)x; // Avoid potential unused variable warnings.

        // See notes in the other splice function regarding this assertion.
        // EASTL_ASSERT(mAllocator == x.mAllocator); // Disabled because our member sort function uses splice but with allocators that may be unequal. There isn't a simple workaround aside from disabling this assert.

        // Disabled until the performance hit of this code is deemed worthwhile and until we are sure we want to disallow unequal allocators.
        //#if EASTL_EXCEPTIONS_ENABLED
        //    if(EASTL_UNLIKELY(!(mAllocator == x.mAllocator)))
        //        throw std::runtime_error("list::splice -- unequal allocators");
        //#endif

        #if EASTL_LIST_SIZE_CACHE
            const size_type n = (size_type)eastl::distance(first, last);

            if(n)
            {
                ((ListNodeBase*)position.mpNode)->splice((ListNodeBase*)first.mpNode, (ListNodeBase*)last.mpNode);
                mSize += n;
                x.mSize -= n;
            }
        #else
            if(first != last)
                ((ListNodeBase*)position.mpNode)->splice((ListNodeBase*)first.mpNode, (ListNodeBase*)last.mpNode);
        #endif
    }


    template <typename T, typename Allocator>
    inline void list<T, Allocator>::swap(this_type& x)
    {
        if(mAllocator == x.mAllocator) // If allocators are equivalent...
        {
            // We leave mAllocator as-is.
            ListNodeBase::swap((ListNodeBase&)mNode, (ListNodeBase&)x.mNode); // We need to implement a special swap because we can't do a shallow swap.

            #if EASTL_LIST_SIZE_CACHE
                eastl::swap(mSize, x.mSize);
            #endif
        }
        else // else swap the contents.
        {
            const this_type temp(*this); // Can't call eastl::swap because that would
            *this = x;                   // itself call this member swap function.
            x     = temp;

            // Alternative implementation:
            //const iterator pos((ListNodeBase*)mNode.mpNext);
            //splice(pos, x);
            //x.splice(x.begin(), *this, pos, iterator((ListNodeBase*)&mNode));
        }
    }


    template <typename T, typename Allocator>
    void list<T, Allocator>::merge(this_type& x)
    {
        if(this != &x)
        {
            iterator       first(begin());
            iterator       firstX(x.begin());
            const iterator last(end());
            const iterator lastX(x.end());

            while((first != last) && (firstX != lastX))
            {
                if(*firstX < *first)
                {
                    iterator next(firstX);

                    splice(first, x, firstX, ++next);
                    firstX = next;
                }
                else
                    ++first;
            }

            if(firstX != lastX)
                splice(last, x, firstX, lastX);
        }
    }


    template <typename T, typename Allocator>
    template <typename Compare>
    void list<T, Allocator>::merge(this_type& x, Compare compare)
    {
        if(this != &x)
        {
            iterator       first(begin());
            iterator       firstX(x.begin());
            const iterator last(end());
            const iterator lastX(x.end());

            while((first != last) && (firstX != lastX))
            {
                if(compare(*firstX, *first))
                {
                    iterator next(firstX);

                    splice(first, x, firstX, ++next);
                    firstX = next;
                }
                else
                    ++first;
            }

            if(firstX != lastX)
                splice(last, x, firstX, lastX);
        }
    }


    template <typename T, typename Allocator>
    void list<T, Allocator>::unique()
    {
        iterator       first(begin());
        const iterator last(end());

        if(first != last)
        {
            iterator next(first);

            while(++next != last)
            {
                if(*first == *next)
                    DoErase((ListNodeBase*)next.mpNode);
                else
                    first = next;
                next = first;
            }
        }
    }


    template <typename T, typename Allocator>
    template <typename BinaryPredicate>
    void list<T, Allocator>::unique(BinaryPredicate predicate)
    {
        iterator       first(begin());
        const iterator last(end());

        if(first != last)
        {
            iterator next(first);

            while(++next != last)
            {
                if(predicate(*first, *next))
                    DoErase((ListNodeBase*)next.mpNode);
                else
                    first = next;
                next = first;
            }
        }
    }


    template <typename T, typename Allocator>
    void list<T, Allocator>::sort()
    {
        // We implement the algorithm employed by Chris Caulfield whereby we use recursive
        // function calls to sort the list. The sorting of a very large list may fail due to stack overflow
        // if the stack is exhausted. The limit depends on the platform and the avaialable stack space.

        // Easier-to-understand version of the 'if' statement:
        // iterator i(begin());
        // if((i != end()) && (++i != end())) // If the size is >= 2 (without calling the more expensive size() function)...

        // Faster, more inlinable version of the 'if' statement:
        if((static_cast<node_type*>(mNode.mpNext) != &mNode) &&
           (static_cast<node_type*>(mNode.mpNext) != static_cast<node_type*>(mNode.mpPrev)))
        {
            // We may have a stack space problem here if sizeof(this_type) is large (usually due to 
            // usage of a fixed_list). The only current resolution is to find an alternative way of 
            // doing things. I (Paul Pedriana) believe that the best long-term solution to this problem
            // is to revise this sort function to not use this_type but instead use a ListNodeBase
            // which involves no allocators and sort at that level, entirely with node pointers.

            // Split the array into 2 roughly equal halves.
            this_type leftList(get_allocator());     // This should cause no memory allocation.
            this_type rightList(get_allocator());

            // We find an iterator which is in the middle of the list. The fastest way to do 
            // this is to iterate from the base node both forwards and backwards with two 
            // iterators and stop when they meet each other. Recall that our size() function 
            // is not O(1) but is instead O(n), at least when EASTL_LIST_SIZE_CACHE is disabled.
            #if EASTL_LIST_SIZE_CACHE
                iterator mid(begin());
                eastl::advance(mid, size() / 2);
            #else
                iterator mid(begin()), tail(end());

                while((mid != tail) && (++mid != tail))
                    --tail;
            #endif

            // Move the left half of this into leftList and the right half into rightList.
            leftList.splice(leftList.begin(), *this, begin(), mid);
            rightList.splice(rightList.begin(), *this);

            // Sort the sub-lists.
            leftList.sort();
            rightList.sort();

            // Merge the two halves into this list.
            splice(begin(), leftList);
            merge(rightList);
        }
    }


    template <typename T, typename Allocator>
    template<typename Compare>
    void list<T, Allocator>::sort(Compare compare)
    {
        // We implement the algorithm employed by Chris Caulfield whereby we use recursive
        // function calls to sort the list. The sorting of a very large list may fail due to stack overflow
        // if the stack is exhausted. The limit depends on the platform and the avaialble stack space.

        // Easier-to-understand version of the 'if' statement:
        // iterator i(begin());
        // if((i != end()) && (++i != end())) // If the size is >= 2 (without calling the more expensive size() function)...

        // Faster, more inlinable version of the 'if' statement:
        if((static_cast<node_type*>(mNode.mpNext) != &mNode) &&
           (static_cast<node_type*>(mNode.mpNext) != static_cast<node_type*>(mNode.mpPrev)))
        {
            // We may have a stack space problem here if sizeof(this_type) is large (usually due to 
            // usage of a fixed_list). The only current resolution is to find an alternative way of 
            // doing things. I (Paul Pedriana) believe that the best long-term solution to this problem
            // is to revise this sort function to not use this_type but instead use a ListNodeBase
            // which involves no allocators and sort at that level, entirely with node pointers.

            // Split the array into 2 roughly equal halves.
            this_type leftList(get_allocator());     // This should cause no memory allocation.
            this_type rightList(get_allocator());

            // We find an iterator which is in the middle of the list. The fastest way to do 
            // this is to iterate from the base node both forwards and backwards with two 
            // iterators and stop when they meet each other. Recall that our size() function 
            // is not O(1) but is instead O(n), at least when EASTL_LIST_SIZE_CACHE is disabled.
            #if EASTL_LIST_SIZE_CACHE
                iterator mid(begin());
                eastl::advance(mid, size() / 2);
            #else
                iterator mid(begin()), tail(end());

                while((mid != tail) && (++mid != tail))
                    --tail;
            #endif

            // Move the left half of this into leftList and the right half into rightList.
            leftList.splice(leftList.begin(), *this, begin(), mid);
            rightList.splice(rightList.begin(), *this);

            // Sort the sub-lists.
            leftList.sort(compare);
            rightList.sort(compare);

            // Merge the two halves into this list.
            splice(begin(), leftList);
            merge(rightList, compare);
        }
    }


    template <typename T, typename Allocator>
    inline typename list<T, Allocator>::node_type*
    list<T, Allocator>::DoCreateNode(const value_type& value)
    {
        node_type* const pNode = DoAllocateNode();

        #if EASTL_EXCEPTIONS_ENABLED
            try
            {
                ::new(&pNode->mValue) value_type(value);
            }
            catch(...)
            {
                DoFreeNode(pNode);
                throw;
            }
        #else
            ::new(&pNode->mValue) value_type(value);
        #endif

        return pNode;
    }


    template <typename T, typename Allocator>
    inline typename list<T, Allocator>::node_type*
    list<T, Allocator>::DoCreateNode()
    {
        node_type* const pNode = DoAllocateNode();

        #if EASTL_EXCEPTIONS_ENABLED
            try
            {
                ::new(&pNode->mValue) value_type();
            }
            catch(...)
            {
                DoFreeNode(pNode);
                throw;
            }
        #else
            ::new(&pNode->mValue) value_type;
        #endif

        return pNode;
    }


    template <typename T, typename Allocator>
    template <typename Integer>
    inline void list<T, Allocator>::DoAssign(Integer n, Integer value, true_type)
    {
        DoAssignValues(static_cast<size_type>(n), static_cast<value_type>(value));
    }


    template <typename T, typename Allocator>
    template <typename InputIterator>
    void list<T, Allocator>::DoAssign(InputIterator first, InputIterator last, false_type)
    {
        node_type* pNode = static_cast<node_type*>(mNode.mpNext);

        for(; (pNode != &mNode) && (first != last); ++first)
        {
            pNode->mValue = *first;
            pNode         = static_cast<node_type*>(pNode->mpNext);
        }

        if(first == last)
            erase(iterator((ListNodeBase*)pNode), (ListNodeBase*)&mNode);
        else
            DoInsert((ListNodeBase*)&mNode, first, last, false_type());
    }


    template <typename T, typename Allocator>
    void list<T, Allocator>::DoAssignValues(size_type n, const value_type& value)
    {
        node_type* pNode  = static_cast<node_type*>(mNode.mpNext);

        for(; (pNode != &mNode) && (n > 0); --n)
        {
            pNode->mValue = value;
            pNode         = static_cast<node_type*>(pNode->mpNext);
        }

        if(n)
            DoInsertValues((ListNodeBase*)&mNode, n, value);
        else
            erase(iterator((ListNodeBase*)pNode), (ListNodeBase*)&mNode);
    }


    template <typename T, typename Allocator>
    template <typename Integer>
    inline void list<T, Allocator>::DoInsert(ListNodeBase* pNode, Integer n, Integer value, true_type)
    {
        DoInsertValues(pNode, static_cast<size_type>(n), static_cast<value_type>(value));
    }


    template <typename T, typename Allocator>
    template <typename InputIterator>
    inline void list<T, Allocator>::DoInsert(ListNodeBase* pNode, InputIterator first, InputIterator last, false_type)
    {
        for(; first != last; ++first)
            DoInsertValue(pNode, *first);
    }


    template <typename T, typename Allocator>
    inline void list<T, Allocator>::DoInsertValues(ListNodeBase* pNode, size_type n, const value_type& value)
    {
        for(; n > 0; --n)
            DoInsertValue(pNode, value);
    }


    template <typename T, typename Allocator>
    inline void list<T, Allocator>::DoInsertValue(ListNodeBase* pNode, const value_type& value)
    {
        node_type* const pNodeNew = DoCreateNode(value);
        ((ListNodeBase*)pNodeNew)->insert((ListNodeBase*)pNode);
        #if EASTL_LIST_SIZE_CACHE
            ++mSize;
        #endif
    }


    template <typename T, typename Allocator>
    inline void list<T, Allocator>::DoErase(ListNodeBase* pNode)
    {
        pNode->remove();
        ((node_type*)pNode)->~node_type();
        DoFreeNode(((node_type*)pNode));
        #if EASTL_LIST_SIZE_CACHE
            --mSize;
        #endif
    }


    template <typename T, typename Allocator>
    inline bool list<T, Allocator>::validate() const
    {
        #if EASTL_LIST_SIZE_CACHE
            size_type n = 0;

            for(const_iterator i(begin()), iEnd(end()); i != iEnd; ++i)
                ++n;

            if(n != mSize)
                return false;
        #endif

        // To do: More validation.
        return true;
    }


    template <typename T, typename Allocator>
    inline int list<T, Allocator>::validate_iterator(const_iterator i) const
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

    template <typename T, typename Allocator>
    bool operator==(const list<T, Allocator>& a, const list<T, Allocator>& b)
    {
        typename list<T, Allocator>::const_iterator ia   = a.begin();
        typename list<T, Allocator>::const_iterator ib   = b.begin();
        typename list<T, Allocator>::const_iterator enda = a.end();

        #if EASTL_LIST_SIZE_CACHE
            if(a.size() == b.size())
            {
                while((ia != enda) && (*ia == *ib))
                {
                    ++ia;
                    ++ib;
                }
                return (ia == enda);
            }
            return false;
        #else
            typename list<T, Allocator>::const_iterator endb = b.end();

            while((ia != enda) && (ib != endb) && (*ia == *ib))
            {
                ++ia;
                ++ib;
            }
            return (ia == enda) && (ib == endb);
        #endif
    }

    template <typename T, typename Allocator>
    bool operator<(const list<T, Allocator>& a, const list<T, Allocator>& b)
    {
        return eastl::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
    }

    template <typename T, typename Allocator>
    bool operator!=(const list<T, Allocator>& a, const list<T, Allocator>& b)
    {
        return !(a == b);
    }

    template <typename T, typename Allocator>
    bool operator>(const list<T, Allocator>& a, const list<T, Allocator>& b)
    {
        return b < a;
    }

    template <typename T, typename Allocator>
    bool operator<=(const list<T, Allocator>& a, const list<T, Allocator>& b)
    {
        return !(b < a);
    }

    template <typename T, typename Allocator>
    bool operator>=(const list<T, Allocator>& a, const list<T, Allocator>& b)
    {
        return !(a < b);
    }

    template <typename T, typename Allocator>
    void swap(list<T, Allocator>& a, list<T, Allocator>& b)
    {
        a.swap(b);
    }


} // namespace eastl


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard

























