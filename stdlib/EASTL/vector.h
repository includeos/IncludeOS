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
// EASTL/vector.h
//
// Copyright (c) 2005, Electronic Arts. All rights reserved.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements a vector (array-like container), much like the C++ 
// std::vector class.
// The primary distinctions between this vector and std::vector are:
//    - vector has a couple extension functions that increase performance.
//    - vector can contain objects with alignment requirements. std::vector 
//      cannot do so without a bit of tedious non-portable effort.
//    - vector supports debug memory naming natively.
//    - vector is easier to read, debug, and visualize.
//    - vector is savvy to an environment that doesn't have exception handling,
//      as is sometimes the case with console or embedded environments.
//    - vector has less deeply nested function calls and allows the user to 
//      enable forced inlining in debug builds in order to reduce bloat.
//    - vector<bool> is a vector of boolean values and not a bit vector.
//    - vector guarantees that memory is contiguous and that vector::iterator
//      is nothing more than a pointer to T.
//    - vector has an explicit data() method for obtaining a pointer to storage 
//      which is safe to call even if the block is empty. This avoids the 
//      common &v[0], &v.front(), and &*v.begin() constructs that trigger false 
//      asserts in STL debugging modes.
//    - vector::size_type is defined as eastl_size_t instead of size_t in order to 
//      save memory and run faster on 64 bit systems.
//    - vector data is guaranteed to be contiguous.
//    - vector has a set_capacity() function which frees excess capacity. 
//      The only way to do this with std::vector is via the cryptic non-obvious 
//      trick of using: vector<SomeClass>(x).swap(x);
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_VECTOR_H
#define EASTL_VECTOR_H


#include <EASTL/internal/config.h>
#include <EASTL/allocator.h>
#include <EASTL/type_traits.h>
#include <EASTL/iterator.h>
#include <EASTL/algorithm.h>
#include <EASTL/memory.h>

#ifdef _MSC_VER
#  pragma warning(push, 0)
//#  include <new>
#  include <stddef.h>
#  pragma warning(pop)
#else
//#  include <new>
#  include <stddef.h>
#endif

#if EASTL_EXCEPTIONS_ENABLED
#  ifdef _MSC_VER
#    pragma warning(push, 0)
#  endif
#  include <stdexcept> // std::out_of_range, std::length_error.
#  ifdef _MSC_VER
#    pragma warning(pop)
#  endif
#endif

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4530)  // C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
#  pragma warning(disable: 4345)  // Behavior change: an object of POD type constructed with an initializer of the form () will be default-initialized
#  pragma warning(disable: 4244)  // Argument: conversion from 'int' to 'const eastl::vector<T>::value_type', possible loss of data
#  pragma warning(disable: 4127)  // Conditional expression is constant
#  pragma warning(disable: 4480)  // nonstandard extension used: specifying underlying type for enum
#endif


namespace eastl
{

    /// EASTL_VECTOR_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    ///
#ifndef EASTL_VECTOR_DEFAULT_NAME
#  define EASTL_VECTOR_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " vector" // Unless the user overrides something, this is "EASTL vector".
#endif


    /// EASTL_VECTOR_DEFAULT_ALLOCATOR
    ///
#ifndef EASTL_VECTOR_DEFAULT_ALLOCATOR
#  define EASTL_VECTOR_DEFAULT_ALLOCATOR allocator_type(EASTL_VECTOR_DEFAULT_NAME)
#endif



    /// VectorBase
    ///
    /// The reason we have a VectorBase class is that it makes exception handling
    /// simpler to implement because memory allocation is implemented entirely 
    /// in this class. If a user creates a vector which needs to allocate
    /// memory in the constructor, VectorBase handles it. If an exception is thrown
    /// by the allocator then the exception throw jumps back to the user code and 
    /// no try/catch code need be written in the vector or VectorBase constructor. 
    /// If an exception is thrown in the vector (not VectorBase) constructor, the 
    /// destructor for VectorBase will be called automatically (and free the allocated
    /// memory) before the execution jumps back to the user code.
    /// However, if the vector class were to handle both allocation and initialization
    /// then it would have no choice but to implement an explicit try/catch statement
    /// for all pathways that allocate memory. This increases code size and decreases
    /// performance and makes the code a little harder read and maintain.
    ///
    /// The C++ standard (15.2 paragraph 2) states: 
    ///    "An object that is partially constructed or partially destroyed will
    ///     have destructors executed for all its fully constructed subobjects,
    ///     that is, for subobjects for which the constructor has been completed
    ///     execution and the destructor has not yet begun execution."
    ///
    /// The C++ standard (15.3 paragraph 11) states: 
    ///    "The fully constructed base classes and members of an object shall 
    ///     be destroyed before entering the handler of a function-try-block
    ///     of a constructor or destructor for that block."
    ///
    template <typename T, typename Allocator>
    struct VectorBase
    {
        typedef Allocator    allocator_type;
        typedef eastl_size_t size_type;             // See config.h for the definition of eastl_size_t, which defaults to uint32_t.
        typedef ptrdiff_t    difference_type;

#if defined(_MSC_VER) && (_MSC_VER >= 1400) // _MSC_VER of 1400 means VC8 (VS2005), 1500 means VC9 (VS2008)
            enum : size_type {                      // Use Microsoft enum language extension, allowing for smaller debug symbols than using a static const. Users have been affected by this.
                npos     = (size_type)-1,
                kMaxSize = (size_type)-2
            };
#else
            static const size_type npos     = (size_type)-1;      /// 'npos' means non-valid position or simply non-position.
            static const size_type kMaxSize = (size_type)-2;      /// -1 is reserved for 'npos'. It also happens to be slightly beneficial that kMaxSize is a value less than -1, as it helps us deal with potential integer wraparound issues.
#endif

        enum
        {
            kAlignment       = EASTL_ALIGN_OF(T),
            kAlignmentOffset = 0
        };

    protected:
        T*              mpBegin;
        T*              mpEnd;
        T*              mpCapacity;
        allocator_type  mAllocator;  // To do: Use base class optimization to make this go away.

    public:
        VectorBase();
        VectorBase(const allocator_type& allocator);
        VectorBase(size_type n, const allocator_type& allocator);

       ~VectorBase();

        allocator_type& get_allocator();
        void            set_allocator(const allocator_type& allocator);

    protected:
        T*        DoAllocate(size_type n);
        void      DoFree(T* p, size_type n);
        size_type GetNewCapacity(size_type currentCapacity);

    }; // VectorBase




    /// vector
    ///
    /// Implements a dynamic array.
    ///
    template <typename T, typename Allocator = EASTLAllocatorType>
    class vector : public VectorBase<T, Allocator>
    {
        typedef VectorBase<T, Allocator>                      base_type;
        typedef vector<T, Allocator>                          this_type;

    public:
        typedef T                                             value_type;
        typedef T*                                            pointer;
        typedef const T*                                      const_pointer;
        typedef T&                                            reference;
        typedef const T&                                      const_reference;  // Maintainer note: We want to leave iterator defined as T* -- at least in release builds -- as this gives some algorithms an advantage that optimizers cannot get around.
        typedef T*                                            iterator;         // Note: iterator is simply T* right now, but this will likely change in the future, at least for debug builds. 
        typedef const T*                                      const_iterator;   //       Do not write code that relies on iterator being T*. The reason it will 
        typedef eastl::reverse_iterator<iterator>             reverse_iterator; //       change in the future is that a debugging iterator system will be created.
        typedef eastl::reverse_iterator<const_iterator>       const_reverse_iterator;    
        typedef typename base_type::size_type                 size_type;
        typedef typename base_type::difference_type           difference_type;
        typedef typename base_type::allocator_type            allocator_type;

        using base_type::mpBegin;
        using base_type::mpEnd;
        using base_type::mpCapacity;
        using base_type::mAllocator;
        using base_type::npos;
        using base_type::GetNewCapacity;
        using base_type::DoAllocate;
        using base_type::DoFree;

    public:
        vector();
        explicit vector(const allocator_type& allocator);
        explicit vector(size_type n, const allocator_type& allocator = EASTL_VECTOR_DEFAULT_ALLOCATOR);
        vector(size_type n, const value_type& value, const allocator_type& allocator = EASTL_VECTOR_DEFAULT_ALLOCATOR);
        vector(const this_type& x);

#ifdef EA_COMPILER_HAS_MOVE_SEMANTICS
        vector(this_type&& x);
        this_type& operator =(this_type&& x);
        iterator insert(iterator position, value_type&& value);
        void push_back(value_type&& x);
#  ifdef EA_COMPILER_HAS_VARIADIC_TEMPLATES
        /*
        template<class ... Args>
        iterator emplace(const_iterator pos, Args&& ... args);
        template<class ... Args>
        iterator emplace_back(const_iterator pos, Args&& ... args);
        */
#  endif
#endif

        template <typename InputIterator>
        vector(InputIterator first, InputIterator last); // allocator arg removed because VC7.1 fails on the default arg. To do: Make a second version of this function without a default arg.

       ~vector();

        this_type& operator=(const this_type& x);
        void swap(this_type& x);

        void assign(size_type n, const value_type& value);

        template <typename InputIterator>
        void assign(InputIterator first, InputIterator last);

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
        size_type capacity() const;

        void resize(size_type n, const value_type& value);
        void resize(size_type n);
        void reserve(size_type n);
        void set_capacity(size_type n = base_type::npos);   // Revises the capacity to the user-specified value. Resizes the container to match the capacity if the requested capacity n is less than the current size. If n == npos then the capacity is reallocated (if necessary) such that capacity == size.

        pointer       data();
        const_pointer data() const;

        reference       operator[](size_type n);
        const_reference operator[](size_type n) const;

        reference       at(size_type n);
        const_reference at(size_type n) const;

        reference       front();
        const_reference front() const;

        reference       back();
        const_reference back() const;

        void      push_back(const value_type& value);
        reference push_back();
        void*     push_back_uninitialized();
		
		template< class... Args >
		void emplace_back(Args&&... args)
		{
			if(mpEnd < mpCapacity)
				::new(mpEnd++) value_type(new T(args...));
			else
				DoInsertValue(mpEnd, new T(args...));
		}
		
        void      pop_back();

        iterator insert(iterator position, const value_type& value);
        void     insert(iterator position, size_type n, const value_type& value);

        template <typename InputIterator>
        void insert(iterator position, InputIterator first, InputIterator last);

        iterator erase(iterator position);
        iterator erase(iterator first, iterator last);

        reverse_iterator erase(reverse_iterator position);
        reverse_iterator erase(reverse_iterator first, reverse_iterator last);

        void     clear();
        void     reset();   /// This is a unilateral reset to an initially empty state. No destructors are called, no deallocation occurs.

        bool validate() const;
        int  validate_iterator(const_iterator i) const;

    protected:
        // These functions do the real work of maintaining the vector. You will notice
        // that many of them have the same name but are specialized on iterator_tag
        // (iterator categories). This is because in these cases there is an optimized
        // implementation that can be had for some cases relative to others. Functions
        // which aren't referenced are neither compiled nor linked into the application.

        template <typename ForwardIterator>
        pointer DoRealloc(size_type n, ForwardIterator first, ForwardIterator last);

        template <typename Integer>
        void DoInit(Integer n, Integer value, true_type);

        template <typename InputIterator>
        void DoInit(InputIterator first, InputIterator last, false_type);

        template <typename InputIterator>
        void DoInitFromIterator(InputIterator first, InputIterator last, EASTL_ITC_NS::input_iterator_tag);

        template <typename ForwardIterator>
        void DoInitFromIterator(ForwardIterator first, ForwardIterator last, EASTL_ITC_NS::forward_iterator_tag);

        void DoDestroyValues(pointer first, pointer last);

        template <typename Integer>
        void DoAssign(Integer n, Integer value, true_type);

        template <typename InputIterator>
        void DoAssign(InputIterator first, InputIterator last, false_type);

        void DoAssignValues(size_type n, const value_type& value);

        template <typename InputIterator>
        void DoAssignFromIterator(InputIterator first, InputIterator last, EASTL_ITC_NS::input_iterator_tag);

        template <typename RandomAccessIterator>
        void DoAssignFromIterator(RandomAccessIterator first, RandomAccessIterator last, EASTL_ITC_NS::random_access_iterator_tag);

        template <typename Integer>
        void DoInsert(iterator position, Integer n, Integer value, true_type);

        template <typename InputIterator>
        void DoInsert(iterator position, InputIterator first, InputIterator last, false_type);

        template <typename InputIterator>
        void DoInsertFromIterator(iterator position, InputIterator first, InputIterator last, EASTL_ITC_NS::input_iterator_tag);

        template <typename BidirectionalIterator>
        void DoInsertFromIterator(iterator position, BidirectionalIterator first, BidirectionalIterator last, EASTL_ITC_NS::bidirectional_iterator_tag);

        void DoInsertValues(iterator position, size_type n, const value_type& value);

        void DoInsertValue(iterator position, const value_type& value);
#ifdef EA_COMPILER_HAS_MOVE_SEMANTICS
        void DoInsertValue(iterator position, value_type&& value);
#endif

    }; // class vector






    ///////////////////////////////////////////////////////////////////////
    // VectorBase
    ///////////////////////////////////////////////////////////////////////

    template <typename T, typename Allocator>
    inline VectorBase<T, Allocator>::VectorBase()
        : mpBegin(NULL), 
          mpEnd(NULL),
          mpCapacity(NULL),
          mAllocator(EASTL_VECTOR_DEFAULT_NAME)
    {
    }

    template <typename T, typename Allocator>
    inline VectorBase<T, Allocator>::VectorBase(const allocator_type& allocator)
        : mpBegin(NULL), 
          mpEnd(NULL),
          mpCapacity(NULL),
          mAllocator(allocator)
    {
    }


    template <typename T, typename Allocator>
    inline VectorBase<T, Allocator>::VectorBase(size_type n, const allocator_type& allocator)
        : mAllocator(allocator)
    {
        mpBegin    = DoAllocate(n);
        mpEnd      = mpBegin;
        mpCapacity = mpBegin + n;
    }


    template <typename T, typename Allocator>
    inline VectorBase<T, Allocator>::~VectorBase()
    {
        if(mpBegin)
            EASTLFree(mAllocator, mpBegin, (mpCapacity - mpBegin) * sizeof(T));
    }


    template <typename T, typename Allocator>
    inline typename VectorBase<T, Allocator>::allocator_type&
    VectorBase<T, Allocator>::get_allocator()
    {
        return mAllocator;
    }


    template <typename T, typename Allocator>
    inline void VectorBase<T, Allocator>::set_allocator(const allocator_type& allocator)
    {
        mAllocator = allocator;
    }


    template <typename T, typename Allocator>
    inline T* VectorBase<T, Allocator>::DoAllocate(size_type n)
    {
#if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(n >= 0x80000000))
                EASTL_FAIL_MSG("vector::DoAllocate -- improbably large request.");
#endif

        // If n is zero, then we allocate no memory and just return NULL. 
        // This is fine, as our default ctor initializes with NULL pointers. 
        return n ? (T*)allocate_memory(mAllocator, n * sizeof(T), kAlignment, kAlignmentOffset) : NULL;
    }


    template <typename T, typename Allocator>
    inline void VectorBase<T, Allocator>::DoFree(T* p, size_type n)
    {
        if(p)
            EASTLFree(mAllocator, p, n * sizeof(T)); 
    }


    template <typename T, typename Allocator>
    inline typename VectorBase<T, Allocator>::size_type
    VectorBase<T, Allocator>::GetNewCapacity(size_type currentCapacity)
    {
        // This needs to return a value of at least currentCapacity and at least 1.
        return (currentCapacity > 0) ? (2 * currentCapacity) : 1;
    }




    ///////////////////////////////////////////////////////////////////////
    // vector
    ///////////////////////////////////////////////////////////////////////

    template <typename T, typename Allocator>
    inline vector<T, Allocator>::vector()
        : base_type()
    {
        // Empty
    }


    template <typename T, typename Allocator>
    inline vector<T, Allocator>::vector(const allocator_type& allocator)
        : base_type(allocator)
    {
        // Empty
    }


    template <typename T, typename Allocator>
    inline vector<T, Allocator>::vector(size_type n, const allocator_type& allocator)
        : base_type(n, allocator)
    {
        eastl::uninitialized_fill_n_ptr(mpBegin, n, value_type());
        mpEnd = mpBegin + n;        
    }


    template <typename T, typename Allocator>
    inline vector<T, Allocator>::vector(size_type n, const value_type& value, const allocator_type& allocator)
        : base_type(n, allocator)
    {
        eastl::uninitialized_fill_n_ptr(mpBegin, n, value);
        mpEnd = mpBegin + n;
    }


    template <typename T, typename Allocator>
    inline vector<T, Allocator>::vector(const this_type& x)
        : base_type(x.size(), x.mAllocator)
    {
        mpEnd = eastl::uninitialized_copy_ptr(x.mpBegin, x.mpEnd, mpBegin);
    }


    template <typename T, typename Allocator>
    template <typename InputIterator>
    inline vector<T, Allocator>::vector(InputIterator first, InputIterator last)
        : base_type(EASTL_VECTOR_DEFAULT_ALLOCATOR)
    {
        DoInit(first, last, is_integral<InputIterator>());
    }

#ifdef EA_COMPILER_HAS_MOVE_SEMANTICS
    template <typename T, typename Allocator>
    inline vector<T, Allocator>::vector(vector<T, Allocator>&& x)
           : base_type()
    {
      if(&x == this) { return; }

      swap(x);
    }

    template <typename T, typename Allocator>
    inline vector<T, Allocator>& vector<T, Allocator>::operator =(vector<T, Allocator>&& x)
    {
      if(&x != this) {
        DoDestroyValues(mpBegin, mpEnd);
        DoFree(mpBegin, (size_type)(mpCapacity - mpBegin));

        mpBegin = NULL; 
        mpEnd = NULL;
        mpCapacity = NULL;
        mAllocator = EASTL_VECTOR_DEFAULT_NAME;

        swap(x);
      }
      return *this;
    }

    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::iterator vector<T, Allocator>::insert(
        typename vector<T, Allocator>::iterator const position, T&& value)
    {
#if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY((position < mpBegin) || (position > mpEnd)))
                EASTL_FAIL_MSG("vector::insert -- invalid position");
#endif

        const ptrdiff_t n = position - mpBegin; // Save this because we might reallocate.

        if((mpEnd == mpCapacity) || (position != mpEnd))
            DoInsertValue(position, value);
        else
            ::new(mpEnd++) value_type(value);

        return mpBegin + n;
    }

    template <typename T, typename Allocator>
    inline void vector<T, Allocator>::push_back(T&& value)
    {
        if(mpEnd < mpCapacity)
            ::new(mpEnd++) value_type(std::forward<T>(value));
        else
            DoInsertValue(mpEnd, std::forward<T>(value));
    }

#  ifdef EA_COMPILER_HAS_VARIADIC_TEMPLATES
    /*
    template <typename T, typename Allocator>
    template<class ... Args>
    typename vector<T, Allocator>::iterator vector<T, Allocator>::emplace(const_iterator pos, Args&& ... args);

    template <typename T, typename Allocator>
    template<class ... Args>
    typename vector<T, Allocator>::iterator vector<T, Allocator>::emplace_back(const_iterator pos, Args&& ... args);
    */
#  endif
#endif

    template <typename T, typename Allocator>
    inline vector<T, Allocator>::~vector()
    {
        // Call destructor for the values. Parent class will free the memory.
        DoDestroyValues(mpBegin, mpEnd);
    }


    template <typename T, typename Allocator>
    typename vector<T, Allocator>::this_type&
    vector<T, Allocator>::operator=(const this_type& x)
    {
        if(&x != this)
        {
#if EASTL_ALLOCATOR_COPY_ENABLED
                mAllocator = x.mAllocator;
#endif

            const size_type n = x.size();

            if(n > size_type(mpCapacity - mpBegin)) // If n > capacity ...
            {
                pointer const pNewData = DoRealloc(n, x.mpBegin, x.mpEnd);
                DoDestroyValues(mpBegin, mpEnd);
                DoFree(mpBegin, (size_type)(mpCapacity - mpBegin));
                mpBegin    = pNewData;
                mpCapacity = mpBegin + n;
            }
            else if(n > size_type(mpEnd - mpBegin)) // If size < n <= capacity ...
            {
                eastl::copy(x.mpBegin, x.mpBegin + (mpEnd - mpBegin), mpBegin);
                eastl::uninitialized_copy_ptr(x.mpBegin + (mpEnd - mpBegin), x.mpEnd, mpEnd);
            }
            else // else n <= size
            {
                iterator const position = eastl::copy(x.mpBegin, x.mpEnd, mpBegin);
                DoDestroyValues(position, mpEnd);
            }
            mpEnd = mpBegin + n;
        }
        return *this;
    }


    template <typename T, typename Allocator>
    inline void vector<T, Allocator>::assign(size_type n, const value_type& value)
    {
        DoAssignValues(n, value);
    }


    template <typename T, typename Allocator>
    template <typename InputIterator>                              
    inline void vector<T, Allocator>::assign(InputIterator first, InputIterator last)
    {
        // It turns out that the C++ std::vector<int, int> specifies a two argument
        // version of assign that takes (int size, int value). These are not iterators, 
        // so we need to do a template compiler trick to do the right thing.
        DoAssign(first, last, is_integral<InputIterator>());
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::iterator
    vector<T, Allocator>::begin()
    {
        return mpBegin;
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::const_iterator
    vector<T, Allocator>::begin() const
    {
        return mpBegin;
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::iterator
    vector<T, Allocator>::end()
    {
        return mpEnd;
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::const_iterator
    vector<T, Allocator>::end() const
    {
        return mpEnd;
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::reverse_iterator
    vector<T, Allocator>::rbegin()
    {
        return reverse_iterator(mpEnd);
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::const_reverse_iterator
    vector<T, Allocator>::rbegin() const
    {
        return const_reverse_iterator(mpEnd);
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::reverse_iterator
    vector<T, Allocator>::rend()
    {
        return reverse_iterator(mpBegin);
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::const_reverse_iterator
    vector<T, Allocator>::rend() const
    {
        return const_reverse_iterator(mpBegin);
    }


    template <typename T, typename Allocator>
    bool vector<T, Allocator>::empty() const
    {
        return (mpBegin == mpEnd);
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::size_type
    vector<T, Allocator>::size() const
    {
        return (size_type)(mpEnd - mpBegin);
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::size_type
    vector<T, Allocator>::capacity() const
    {
        return (size_type)(mpCapacity - mpBegin);
    }


    template <typename T, typename Allocator>
    inline void vector<T, Allocator>::resize(size_type n, const value_type& value)
    {
        if(n > (size_type)(mpEnd - mpBegin))  // We expect that more often than not, resizes will be upsizes.
            insert(mpEnd, n - ((size_type)(mpEnd - mpBegin)), value);
        else
            erase(mpBegin + n, mpEnd);
    }


    template <typename T, typename Allocator>
    inline void vector<T, Allocator>::resize(size_type n)
    {
        // Alternative implementation:
        // resize(n, value_type());

        if(n > (size_type)(mpEnd - mpBegin))  // We expect that more often than not, resizes will be upsizes.
            insert(mpEnd, n - ((size_type)(mpEnd - mpBegin)), value_type());
        else
            erase(mpBegin + n, mpEnd);
    }


    template <typename T, typename Allocator>
    void vector<T, Allocator>::reserve(size_type n)
    {
        // If the user wants to reduce the reserved memory, there is the set_capacity function.
        if(n > size_type(mpCapacity - mpBegin)) // If n > capacity ...
        {
            // To consider: fold this reserve implementation with the set_capacity 
            // implementation below. But we need to be careful to not call resize
            // in the implementation, as that would require the user to have a 
            // default constructor, which we are trying to avoid.
            pointer const pNewData = DoRealloc(n, mpBegin, mpEnd);
            DoDestroyValues(mpBegin, mpEnd);
            DoFree(mpBegin, (size_type)(mpCapacity - mpBegin));

            const ptrdiff_t nPrevSize = mpEnd - mpBegin;
            mpBegin    = pNewData;
            mpEnd      = pNewData + nPrevSize;
            mpCapacity = mpBegin + n;
        }
    }


    template <typename T, typename Allocator>
    void vector<T, Allocator>::set_capacity(size_type n)
    {
        if((n == npos) || (n <= (size_type)(mpEnd - mpBegin))) // If new capacity <= size...
        {
            if(n < (size_type)(mpEnd - mpBegin))
                resize(n);

            this_type temp(*this);  // This is the simplest way to accomplish this, 
            swap(temp);             // and it is as efficient as any other.
        }
        else // Else new capacity > size.
        {
            pointer const pNewData = DoRealloc(n, mpBegin, mpEnd);
            DoDestroyValues(mpBegin, mpEnd);
            DoFree(mpBegin, (size_type)(mpCapacity - mpBegin));

            const ptrdiff_t nPrevSize = mpEnd - mpBegin;
            mpBegin    = pNewData;
            mpEnd      = pNewData + nPrevSize;
            mpCapacity = mpBegin + n;
        }
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::pointer
    vector<T, Allocator>::data()
    {
        return mpBegin;
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::const_pointer
    vector<T, Allocator>::data() const
    {
        return mpBegin;
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::reference
    vector<T, Allocator>::operator[](size_type n)
    {
#if EASTL_EMPTY_REFERENCE_ASSERT_ENABLED    // We allow the user to use a reference to v[0] of an empty container.
            if(EASTL_UNLIKELY((n != 0) && (n >= (static_cast<size_type>(mpEnd - mpBegin)))))
                EASTL_FAIL_MSG("vector::operator[] -- out of range");
#elif EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(n >= (static_cast<size_type>(mpEnd - mpBegin))))
                EASTL_FAIL_MSG("vector::operator[] -- out of range");
#endif

        return *(mpBegin + n);
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::const_reference
    vector<T, Allocator>::operator[](size_type n) const
    {
#if EASTL_EMPTY_REFERENCE_ASSERT_ENABLED    // We allow the user to use a reference to v[0] of an empty container.
            if(EASTL_UNLIKELY((n != 0) && (n >= (static_cast<size_type>(mpEnd - mpBegin)))))
                EASTL_FAIL_MSG("vector::operator[] -- out of range");
#elif EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(n >= (static_cast<size_type>(mpEnd - mpBegin))))
                EASTL_FAIL_MSG("vector::operator[] -- out of range");
#endif

        return *(mpBegin + n);
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::reference
    vector<T, Allocator>::at(size_type n)
    {
#if EASTL_EXCEPTIONS_ENABLED
            if(EASTL_UNLIKELY(n >= (static_cast<size_type>(mpEnd - mpBegin))))
                throw std::out_of_range("vector::at -- out of range");
#elif EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(n >= (static_cast<size_type>(mpEnd - mpBegin))))
                EASTL_FAIL_MSG("vector::at -- out of range");
#endif

        return *(mpBegin + n);
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::const_reference
    vector<T, Allocator>::at(size_type n) const
    {
#if EASTL_EXCEPTIONS_ENABLED
            if(EASTL_UNLIKELY(n >= (static_cast<size_type>(mpEnd - mpBegin))))
                throw std::out_of_range("vector::at -- out of range");
#elif EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(n >= (static_cast<size_type>(mpEnd - mpBegin))))
                EASTL_FAIL_MSG("vector::at -- out of range");
#endif

        return *(mpBegin + n);
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::reference
    vector<T, Allocator>::front()
    {
#if EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
            // We allow the user to reference an empty container.
#elif EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(mpEnd <= mpBegin)) // We don't allow the user to reference an empty container.
                EASTL_FAIL_MSG("vector::front -- empty vector");
#endif

        return *mpBegin;
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::const_reference
    vector<T, Allocator>::front() const
    {
#if EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
            // We allow the user to reference an empty container.
#elif EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(mpEnd <= mpBegin)) // We don't allow the user to reference an empty container.
                EASTL_FAIL_MSG("vector::front -- empty vector");
#endif

        return *mpBegin;
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::reference
    vector<T, Allocator>::back()
    {
#if EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
            // We allow the user to reference an empty container.
#elif EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(mpEnd <= mpBegin)) // We don't allow the user to reference an empty container.
                EASTL_FAIL_MSG("vector::back -- empty vector");
#endif

        return *(mpEnd - 1);
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::const_reference
    vector<T, Allocator>::back() const
    {
#if EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
            // We allow the user to reference an empty container.
#elif EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(mpEnd <= mpBegin)) // We don't allow the user to reference an empty container.
                EASTL_FAIL_MSG("vector::back -- empty vector");
#endif

        return *(mpEnd - 1);
    }


    template <typename T, typename Allocator>
    inline void vector<T, Allocator>::push_back(const value_type& value)
    {
        if(mpEnd < mpCapacity)
            ::new(mpEnd++) value_type(value);
        else
            DoInsertValue(mpEnd, value);
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::reference
    vector<T, Allocator>::push_back()
    {
        if(mpEnd < mpCapacity)
            ::new(mpEnd++) value_type();
        else // Note that in this case we create a temporary, which is less desirable.
            DoInsertValue(mpEnd, value_type());

        return *(mpEnd - 1); // Same as return back();
    }


    template <typename T, typename Allocator>
    inline void* vector<T, Allocator>::push_back_uninitialized()
    {
        if(mpEnd == mpCapacity)
        {
            const size_type newSize = (size_type)(mpEnd - mpBegin) + 1;
            reserve(newSize);
        }
 
        return mpEnd++;
    }


    template <typename T, typename Allocator>
    inline void vector<T, Allocator>::pop_back()
    {
#if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(mpEnd <= mpBegin))
                EASTL_FAIL_MSG("vector::pop_back -- empty vector");
#endif

        --mpEnd;
        mpEnd->~value_type();
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::iterator
    vector<T, Allocator>::insert(iterator position, const value_type& value)
    {
#if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY((position < mpBegin) || (position > mpEnd)))
                EASTL_FAIL_MSG("vector::insert -- invalid position");
#endif

        const ptrdiff_t n = position - mpBegin; // Save this because we might reallocate.

        if((mpEnd == mpCapacity) || (position != mpEnd))
            DoInsertValue(position, value);
        else
            ::new(mpEnd++) value_type(value);

        return mpBegin + n;
    }


    template <typename T, typename Allocator>
    inline void vector<T, Allocator>::insert(iterator position, size_type n, const value_type& value)
    {
        DoInsertValues(position, n, value);
    }


    template <typename T, typename Allocator>
    template <typename InputIterator>
    inline void vector<T, Allocator>::insert(iterator position, InputIterator first, InputIterator last)
    {
        DoInsert(position, first, last, is_integral<InputIterator>());
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::iterator
    vector<T, Allocator>::erase(iterator position)
    {
#if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY((position < mpBegin) || (position >= mpEnd)))
                EASTL_FAIL_MSG("vector::erase -- invalid position");
#endif

        if((position + 1) < mpEnd)
            eastl::copy(position + 1, mpEnd, position);
        --mpEnd;
        mpEnd->~value_type();
        return position;
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::iterator
    vector<T, Allocator>::erase(iterator first, iterator last)
    {
#if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY((first < mpBegin) || (first > mpEnd) || (last < mpBegin) || (last > mpEnd) || (last < first)))
                EASTL_FAIL_MSG("vector::erase -- invalid position");
#endif
 
        //#if 0 
            // Reference implementation, known to be correct:
            iterator const position = eastl::copy(last, mpEnd, first);
            DoDestroyValues(position, mpEnd);
            mpEnd -= (last - first);
        //#else 
        //    To do: Test this.
        //    // Implementation that has an optimization for memcpy which eastl::copy cannot do (the best it can do is memmove).
        //    iterator position;
        //    T* const pEnd = mpEnd - (last - first);
        //
        //    if((pEnd <= last) && eastl::has_trivial_assign<value_type>::value) // If doing a non-overlapping copy and the data is memcpy-able
        //    {
        //        const size_t size = (size_t)((uintptr_t)mpEnd-(uintptr_t)last);
        //        position = (T*)((uintptr_t)memcpy(first, last, size) + size);
        //    }
        //    else
        //        position = eastl::copy(last, mpEnd, first);
        //
        //    DoDestroyValues(position, mpEnd);
        //    mpEnd = pEnd;
        //#endif
 
        return first;
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::reverse_iterator
    vector<T, Allocator>::erase(reverse_iterator position)
    {
        return reverse_iterator(erase((++position).base()));
    }


    template <typename T, typename Allocator>
    inline typename vector<T, Allocator>::reverse_iterator
    vector<T, Allocator>::erase(reverse_iterator first, reverse_iterator last)
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
    inline void vector<T, Allocator>::clear()
    {
        DoDestroyValues(mpBegin, mpEnd);
        mpEnd = mpBegin;
    }


    template <typename T, typename Allocator>
    inline void vector<T, Allocator>::reset()
    {
        // The reset function is a special extension function which unilaterally 
        // resets the container to an empty state without freeing the memory of 
        // the contained objects. This is useful for very quickly tearing down a 
        // container built into scratch memory.
        mpBegin = mpEnd = mpCapacity = NULL;
    }


    template <typename T, typename Allocator>
    inline void vector<T, Allocator>::swap(this_type& x)
    {
        if(mAllocator == x.mAllocator) // If allocators are equivalent...
        {
            // We leave mAllocator as-is.
            eastl::swap(mpBegin,     x.mpBegin);
            eastl::swap(mpEnd,       x.mpEnd);
            eastl::swap(mpCapacity,  x.mpCapacity);
        }
        else // else swap the contents.
        {
            const this_type temp(*this); // Can't call eastl::swap because that would
            *this = x;                   // itself call this member swap function.
            x     = temp;
        }
    }


    template <typename T, typename Allocator>
    template <typename ForwardIterator>
    inline typename vector<T, Allocator>::pointer
    vector<T, Allocator>::DoRealloc(size_type n, ForwardIterator first, ForwardIterator last)
    {
        T* const p = DoAllocate(n);
        eastl::uninitialized_copy_ptr(first, last, p);
        return p;
    }


    template <typename T, typename Allocator>
    template <typename Integer>
    inline void vector<T, Allocator>::DoInit(Integer n, Integer value, true_type)
    {
        mpBegin    = DoAllocate((size_type)n);
        mpCapacity = mpBegin + n;
        mpEnd      = mpCapacity;
        eastl::uninitialized_fill_n_ptr<value_type, Integer>(mpBegin, n, value);
    }


    template <typename T, typename Allocator>
    template <typename InputIterator>
    inline void vector<T, Allocator>::DoInit(InputIterator first, InputIterator last, false_type)
    {
        typedef typename eastl::iterator_traits<InputIterator>:: iterator_category IC;
        DoInitFromIterator(first, last, IC());
    }


    template <typename T, typename Allocator>
    template <typename InputIterator>
    inline void vector<T, Allocator>::DoInitFromIterator(InputIterator first, InputIterator last, EASTL_ITC_NS::input_iterator_tag)
    {
        for(; first < last; ++first)  // InputIterators by definition actually only allow you to iterate through them once.
            push_back(*first);        // Thus the standard *requires* that we do this (inefficient) implementation.
    }                                 // Luckily, InputIterators are in practice almost never used, so this code will likely never get executed.


    template <typename T, typename Allocator>
    template <typename ForwardIterator>
    inline void vector<T, Allocator>::DoInitFromIterator(ForwardIterator first, ForwardIterator last, EASTL_ITC_NS::forward_iterator_tag)
    {
        const size_type n = (size_type)eastl::distance(first, last);
        mpBegin    = DoAllocate(n);
        mpCapacity = mpBegin + n;
        mpEnd      = mpCapacity;
        eastl::uninitialized_copy_ptr(first, last, mpBegin);
    }


    template <typename T, typename Allocator>
    inline void vector<T, Allocator>::DoDestroyValues(pointer first, pointer last)
    {
        for(; first < last; ++first) // In theory, this could be an external function that works on an iterator.
            first->~value_type();
    }


    template <typename T, typename Allocator>
    template <typename Integer>
    inline void vector<T, Allocator>::DoAssign(Integer n, Integer value, true_type)
    {
        DoAssignValues(static_cast<size_type>(n), static_cast<value_type>(value));
    }


    template <typename T, typename Allocator>
    template <typename InputIterator>
    inline void vector<T, Allocator>::DoAssign(InputIterator first, InputIterator last, false_type)
    {
        typedef typename eastl::iterator_traits<InputIterator>::iterator_category IC;
        DoAssignFromIterator(first, last, IC());
    }


    template <typename T, typename Allocator>
    void vector<T, Allocator>::DoAssignValues(size_type n, const value_type& value)
    {
        if(n > size_type(mpCapacity - mpBegin)) // If n > capacity ...
        {
            this_type temp(n, value, mAllocator); // We have little choice but to reallocate with new memory.
            swap(temp);
        }
        else if(n > size_type(mpEnd - mpBegin)) // If n > size ...
        {
            eastl::fill(mpBegin, mpEnd, value);
            eastl::uninitialized_fill_n_ptr(mpEnd, n - size_type(mpEnd - mpBegin), value);
            mpEnd += n - size_type(mpEnd - mpBegin);
        }
        else // else 0 <= n <= size
        {
            eastl::fill_n(mpBegin, n, value);
            erase(mpBegin + n, mpEnd);
        }
    }


    template <typename T, typename Allocator>
    template <typename InputIterator>
    void vector<T, Allocator>::DoAssignFromIterator(InputIterator first, InputIterator last, EASTL_ITC_NS::input_iterator_tag)
    {
        iterator position(mpBegin);

        while((position != mpEnd) && (first != last))
        {
            *position = *first;
            ++first;
            ++position;
        }
        if(first == last)
            erase(position, mpEnd);
        else
            insert(mpEnd, first, last);
    }


    template <typename T, typename Allocator>
    template <typename RandomAccessIterator>
    void vector<T, Allocator>::DoAssignFromIterator(RandomAccessIterator first, RandomAccessIterator last, EASTL_ITC_NS::random_access_iterator_tag)
    {
        const size_type n = (size_type)eastl::distance(first, last);

        if(n > size_type(mpCapacity - mpBegin)) // If n > capacity ...
        {
            pointer const pNewData = DoRealloc(n, first, last);
            DoDestroyValues(mpBegin, mpEnd);
            DoFree(mpBegin, (size_type)(mpCapacity - mpBegin));

            mpBegin    = pNewData;
            mpEnd      = mpBegin + n;
            mpCapacity = mpEnd;
        }
        else if(n <= size_type(mpEnd - mpBegin)) // If n <= size ...
        {
            pointer const pNewEnd = eastl::copy(first, last, mpBegin); // Since we are copying to mpBegin, we don't have to worry about needing copy_backward or a memmove-like copy (as opposed to memcpy-like copy).
            DoDestroyValues(pNewEnd, mpEnd);
            mpEnd = pNewEnd;
        }
        else // else size < n <= capacity
        {
            RandomAccessIterator position = first + (mpEnd - mpBegin);
            eastl::copy(first, position, mpBegin); // Since we are copying to mpBegin, we don't have to worry about needing copy_backward or a memmove-like copy (as opposed to memcpy-like copy).
            mpEnd = eastl::uninitialized_copy_ptr(position, last, mpEnd);
        }
    }


    template <typename T, typename Allocator>
    template <typename Integer>
    inline void vector<T, Allocator>::DoInsert(iterator position, Integer n, Integer value, true_type)
    {
        DoInsertValues(position, static_cast<size_type>(n), static_cast<value_type>(value));
    }


    template <typename T, typename Allocator>
    template <typename InputIterator>
    inline void vector<T, Allocator>::DoInsert(iterator position, InputIterator first, InputIterator last, false_type)
    {
        typedef typename eastl::iterator_traits<InputIterator>::iterator_category IC;
        DoInsertFromIterator(position, first, last, IC());
    }


    template <typename T, typename Allocator>
    template <typename InputIterator>
    inline void vector<T, Allocator>::DoInsertFromIterator(iterator position, InputIterator first, InputIterator last, EASTL_ITC_NS::input_iterator_tag)
    {
        for(; first != last; ++first, ++position)
            position = insert(position, *first);
    }


    template <typename T, typename Allocator>
    template <typename BidirectionalIterator>
    void vector<T, Allocator>::DoInsertFromIterator(iterator position, BidirectionalIterator first, BidirectionalIterator last, EASTL_ITC_NS::bidirectional_iterator_tag)
    {
#if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY((position < mpBegin) || (position > mpEnd)))
                EASTL_FAIL_MSG("vector::insert -- invalid position");
#endif

        if(first != last)
        {
            const size_type n = (size_type)eastl::distance(first, last);

            if(n <= size_type(mpCapacity - mpEnd)) // If n fits within the existing capacity...
            {
                const size_type nExtra = static_cast<size_type>(mpEnd - position);
                const pointer   pEnd   = mpEnd;

                if(n < nExtra)
                {
                    eastl::uninitialized_copy_ptr(mpEnd - n, mpEnd, mpEnd);
                    mpEnd += n;
                    eastl::copy_backward(position, pEnd - n, pEnd); // We need copy_backward because of potential overlap issues.
                    eastl::copy(first, last, position);
                }
                else
                {
                    BidirectionalIterator fiTemp = first;
                    eastl::advance(fiTemp, nExtra);
                    eastl::uninitialized_copy_ptr(fiTemp, last, mpEnd);
                    mpEnd += n - nExtra;
                    eastl::uninitialized_copy_ptr(position, pEnd, mpEnd);
                    mpEnd += nExtra;
                    eastl::copy_backward(first, fiTemp, position + nExtra);
                }
            }
            else // else we need to expand our capacity.
            {
                const size_type nPrevSize = size_type(mpEnd - mpBegin);
                const size_type nGrowSize = GetNewCapacity(nPrevSize);
                const size_type nNewSize  = nGrowSize > (nPrevSize + n) ? nGrowSize : (nPrevSize + n);
                pointer const   pNewData  = DoAllocate(nNewSize);

#if EASTL_EXCEPTIONS_ENABLED
                    pointer pNewEnd = pNewData;
                    try
                    {
                        pNewEnd = eastl::uninitialized_copy_ptr(mpBegin, position, pNewData);
                        pNewEnd = eastl::uninitialized_copy_ptr(first, last, pNewEnd);
                        pNewEnd = eastl::uninitialized_copy_ptr(position, mpEnd, pNewEnd);
                    }
                    catch(...)
                    {
                        DoDestroyValues(pNewData, pNewEnd);
                        DoFree(pNewData, nNewSize);
                        throw;
                    }
#else
                    pointer pNewEnd = eastl::uninitialized_copy_ptr(mpBegin, position, pNewData);
                    pNewEnd         = eastl::uninitialized_copy_ptr(first, last, pNewEnd);
                    pNewEnd         = eastl::uninitialized_copy_ptr(position, mpEnd, pNewEnd);
#endif

                DoDestroyValues(mpBegin, mpEnd);
                DoFree(mpBegin, (size_type)(mpCapacity - mpBegin));

                mpBegin    = pNewData;
                mpEnd      = pNewEnd;
                mpCapacity = pNewData + nNewSize;
            }
        }
    }


    template <typename T, typename Allocator>
    void vector<T, Allocator>::DoInsertValues(iterator position, size_type n, const value_type& value)
    {
#if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY((position < mpBegin) || (position > mpEnd)))
                EASTL_FAIL_MSG("vector::insert -- invalid position");
#endif

        if(n <= size_type(mpCapacity - mpEnd)) // If n is <= capacity...
        {
            if(n > 0) // To do: See if there is a way we can eliminate this 'if' statement.
            {
                // To consider: Make this algorithm work more like DoInsertValue whereby a pointer to value is used.
                const value_type temp  = value;
                const size_type nExtra = static_cast<size_type>(mpEnd - position);
                const pointer pEnd     = mpEnd;

                if(n < nExtra)
                {
                    eastl::uninitialized_copy_ptr(mpEnd - n, mpEnd, mpEnd);
                    mpEnd += n;
                    eastl::copy_backward(position, pEnd - n, pEnd); // We need copy_backward because of potential overlap issues.
                    eastl::fill(position, position + n, temp);
                }
                else
                {
                    eastl::uninitialized_fill_n_ptr(mpEnd, n - nExtra, temp);
                    mpEnd += n - nExtra;
                    eastl::uninitialized_copy_ptr(position, pEnd, mpEnd);
                    mpEnd += nExtra;
                    eastl::fill(position, pEnd, temp);
                }
            }
        }
        else // else n > capacity
        {
            const size_type nPrevSize = size_type(mpEnd - mpBegin);
            const size_type nGrowSize = GetNewCapacity(nPrevSize);
            const size_type nNewSize  = nGrowSize > (nPrevSize + n) ? nGrowSize : (nPrevSize + n);
            pointer const pNewData    = DoAllocate(nNewSize);

#if EASTL_EXCEPTIONS_ENABLED
                pointer pNewEnd = pNewData;
                try
                {
                    pNewEnd = eastl::uninitialized_copy_ptr(mpBegin, position, pNewData);
                    eastl::uninitialized_fill_n_ptr(pNewEnd, n, value);
                    pNewEnd = eastl::uninitialized_copy_ptr(position, mpEnd, pNewEnd + n);
                }
                catch(...)
                {
                    DoDestroyValues(pNewData, pNewEnd);
                    DoFree(pNewData, nNewSize);
                    throw;
                }
#else
                pointer pNewEnd = eastl::uninitialized_copy_ptr(mpBegin, position, pNewData);
                eastl::uninitialized_fill_n_ptr(pNewEnd, n, value);
                pNewEnd = eastl::uninitialized_copy_ptr(position, mpEnd, pNewEnd + n);
#endif

            DoDestroyValues(mpBegin, mpEnd);
            DoFree(mpBegin, (size_type)(mpCapacity - mpBegin));

            mpBegin    = pNewData;
            mpEnd      = pNewEnd;
            mpCapacity = pNewData + nNewSize;
        }
    }


    template <typename T, typename Allocator>
    void vector<T, Allocator>::DoInsertValue(iterator position, const value_type& value)
    {
#if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY((position < mpBegin) || (position > mpEnd)))
                EASTL_FAIL_MSG("vector::insert -- invalid position");
#endif

        if(mpEnd != mpCapacity) // If size < capacity ...
        {
            // EASTL_ASSERT(position < mpEnd); // We don't call this function unless position is less than end, and the code directly below relies on this.
            // We need to take into account the possibility that value may come from within the vector itself.
            const T* pValue = &value;
            if((pValue >= position) && (pValue < mpEnd)) // If value comes from within the range to be moved...
                ++pValue;
            ::new(mpEnd) value_type(*(mpEnd - 1));
            eastl::copy_backward(position, mpEnd - 1, mpEnd); // We need copy_backward because of potential overlap issues.
            *position = *pValue;
            ++mpEnd;
        }
        else // else (size == capacity)
        {
            const size_type nPrevSize = size_type(mpEnd - mpBegin);
            const size_type nNewSize  = GetNewCapacity(nPrevSize);
            pointer const   pNewData  = DoAllocate(nNewSize);

#if EASTL_EXCEPTIONS_ENABLED
                pointer pNewEnd = pNewData;
                try
                {
                    pNewEnd = eastl::uninitialized_copy_ptr(mpBegin, position, pNewData);
                    ::new(pNewEnd) value_type(value);
                    pNewEnd = eastl::uninitialized_copy_ptr(position, mpEnd, ++pNewEnd);
                }
                catch(...)
                {
                    DoDestroyValues(pNewData, pNewEnd);
                    DoFree(pNewData, nNewSize);
                    throw;
                }
#else
                pointer pNewEnd = eastl::uninitialized_copy_ptr(mpBegin, position, pNewData);
                ::new(pNewEnd) value_type(value);
                pNewEnd = eastl::uninitialized_copy_ptr(position, mpEnd, ++pNewEnd);
#endif

            DoDestroyValues(mpBegin, mpEnd);
            DoFree(mpBegin, (size_type)(mpCapacity - mpBegin));

            mpBegin    = pNewData;
            mpEnd      = pNewEnd;
            mpCapacity = pNewData + nNewSize;
        }
    }

#ifdef EA_COMPILER_HAS_MOVE_SEMANTICS
    template <typename T, typename Allocator>
    void vector<T, Allocator>::DoInsertValue(iterator position, value_type&& value)
    {
#if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY((position < mpBegin) || (position > mpEnd)))
                EASTL_FAIL_MSG("vector::insert -- invalid position");
#endif

        if(mpEnd != mpCapacity) // If size < capacity ...
        {
            // EASTL_ASSERT(position < mpEnd); // We don't call this function unless position is less than end, and the code directly below relies on this.
            // We need to take into account the possibility that value may come from within the vector itself.
            const T* pValue = &value;
            if((pValue >= position) && (pValue < mpEnd)) // If value comes from within the range to be moved...
                ++pValue;
            ::new(mpEnd) value_type(std::forward<T>(*(mpEnd - 1)));
            eastl::copy_backward(position, mpEnd - 1, mpEnd); // We need copy_backward because of potential overlap issues.
            *position = *pValue;
            ++mpEnd;
        }
        else // else (size == capacity)
        {
            const size_type nPrevSize = size_type(mpEnd - mpBegin);
            const size_type nNewSize  = GetNewCapacity(nPrevSize);
            pointer const   pNewData  = DoAllocate(nNewSize);

#if EASTL_EXCEPTIONS_ENABLED
                pointer pNewEnd = pNewData;
                try
                {
                    pNewEnd = eastl::uninitialized_copy_ptr(mpBegin, position, pNewData);
                    ::new(pNewEnd) value_type(value);
                    pNewEnd = eastl::uninitialized_copy_ptr(position, mpEnd, ++pNewEnd);
                }
                catch(...)
                {
                    DoDestroyValues(pNewData, pNewEnd);
                    DoFree(pNewData, nNewSize);
                    throw;
                }
#else
                pointer pNewEnd = eastl::uninitialized_copy_ptr(mpBegin, position, pNewData);
                ::new(pNewEnd) value_type(std::forward<T>(value));
                pNewEnd = eastl::uninitialized_copy_ptr(position, mpEnd, ++pNewEnd);
#endif

            DoDestroyValues(mpBegin, mpEnd);
            DoFree(mpBegin, (size_type)(mpCapacity - mpBegin));

            mpBegin    = pNewData;
            mpEnd      = pNewEnd;
            mpCapacity = pNewData + nNewSize;
        }
    }
#endif

    template <typename T, typename Allocator>
    inline bool vector<T, Allocator>::validate() const
    {
        if(mpEnd < mpBegin)
            return false;
        if(mpCapacity < mpEnd)
            return false;
        return true;
    }


    template <typename T, typename Allocator>
    inline int vector<T, Allocator>::validate_iterator(const_iterator i) const
    {
        if(i >= mpBegin)
        {
            if(i < mpEnd)
                return (isf_valid | isf_current | isf_can_dereference);

            if(i <= mpEnd)
                return (isf_valid | isf_current);
        }

        return isf_none;
    }



    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template <typename T, typename Allocator>
    inline bool operator==(const vector<T, Allocator>& a, const vector<T, Allocator>& b)
    {
        return ((a.size() == b.size()) && equal(a.begin(), a.end(), b.begin()));
    }


    template <typename T, typename Allocator>
    inline bool operator!=(const vector<T, Allocator>& a, const vector<T, Allocator>& b)
    {
        return ((a.size() != b.size()) || !equal(a.begin(), a.end(), b.begin()));
    }


    template <typename T, typename Allocator>
    inline bool operator<(const vector<T, Allocator>& a, const vector<T, Allocator>& b)
    {
        return lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
    }


    template <typename T, typename Allocator>
    inline bool operator>(const vector<T, Allocator>& a, const vector<T, Allocator>& b)
    {
        return b < a;
    }


    template <typename T, typename Allocator>
    inline bool operator<=(const vector<T, Allocator>& a, const vector<T, Allocator>& b)
    {
        return !(b < a);
    }


    template <typename T, typename Allocator>
    inline bool operator>=(const vector<T, Allocator>& a, const vector<T, Allocator>& b)
    {
        return !(a < b);
    }


    template <typename T, typename Allocator>
    inline void swap(vector<T, Allocator>& a, vector<T, Allocator>& b)
    {
        a.swap(b);
    }


} // namespace eastl


#ifdef _MSC_VER
#  pragma warning(pop)
#endif


#endif // Header include guard










