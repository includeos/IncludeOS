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
// EASTL/iterator.h
//
// Copyright (c) 2005, Electronic Arts. All rights reserved.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_ITERATOR_H
#define EASTL_ITERATOR_H


#include <EASTL/internal/config.h>

#ifdef _MSC_VER
    #pragma warning(push, 0)
#endif

#include <stddef.h>

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

// If the user has specified that we use std iterator
// categories instead of EASTL iterator categories,
// then #include <iterator>.
#if EASTL_STD_ITERATOR_CATEGORY_ENABLED
    #ifdef _MSC_VER
        #pragma warning(push, 0)
    #endif
    #include <iterator>                 
    #ifdef _MSC_VER
        #pragma warning(pop)
    #endif
#endif                                  


#ifdef _MSC_VER
    #pragma warning(push)           // VC++ generates a bogus warning that you cannot code away.
    #pragma warning(disable: 4619)  // There is no warning number 'number'.
    #pragma warning(disable: 4217)  // Member template functions cannot be used for copy-assignment or copy-construction.
#elif defined(__SNC__)
    #pragma control %push diag
    #pragma diag_suppress=187       // Pointless comparison of unsigned integer with zero
#endif


namespace eastl
{
    /// iterator_status_flag
    /// 
    /// Defines the validity status of an iterator. This is primarily used for 
    /// iterator validation in debug builds. These are implemented as OR-able 
    /// flags (as opposed to mutually exclusive values) in order to deal with 
    /// the nature of iterator status. In particular, an iterator may be valid
    /// but not dereferencable, as in the case with an iterator to container end().
    /// An iterator may be valid but also dereferencable, as in the case with an
    /// iterator to container begin().
    ///
    enum iterator_status_flag
    {
        isf_none            = 0x00, /// This is called none and not called invalid because it is not strictly the opposite of invalid.
        isf_valid           = 0x01, /// The iterator is valid, which means it is in the range of [begin, end].
        isf_current         = 0x02, /// The iterator is valid and points to the same element it did when created. For example, if an iterator points to vector::begin() but an element is inserted at the front, the iterator is valid but not current. Modification of elements in place do not make iterators non-current.
        isf_can_dereference = 0x04  /// The iterator is dereferencable, which means it is in the range of [begin, end). It may or may not be current.
    };



    // The following declarations are taken directly from the C++ standard document.
    //    input_iterator_tag, etc.
    //    iterator
    //    iterator_traits
    //    reverse_iterator

    // Iterator categories
    // Every iterator is defined as belonging to one of the iterator categories that
    // we define here. These categories come directly from the C++ standard.
    #if !EASTL_STD_ITERATOR_CATEGORY_ENABLED // If we are to use our own iterator category definitions...
        struct input_iterator_tag { };
        struct output_iterator_tag { };
        struct forward_iterator_tag       : public input_iterator_tag { };
        struct bidirectional_iterator_tag : public forward_iterator_tag { };
        struct random_access_iterator_tag : public bidirectional_iterator_tag { };
        struct contiguous_iterator_tag    : public random_access_iterator_tag { };  // Extension to the C++ standard. Contiguous ranges are more than random access, they are physically contiguous.
    #endif


    // struct iterator
    template <typename Category, typename T, typename Distance = ptrdiff_t, 
              typename Pointer = T*, typename Reference = T&>
    struct iterator
    {
        typedef Category  iterator_category;
        typedef T         value_type;
        typedef Distance  difference_type;
        typedef Pointer   pointer;
        typedef Reference reference;
    };


    // struct iterator_traits
    template <typename Iterator>
    struct iterator_traits
    {
        typedef typename Iterator::iterator_category iterator_category;
        typedef typename Iterator::value_type        value_type;
        typedef typename Iterator::difference_type   difference_type;
        typedef typename Iterator::pointer           pointer;
        typedef typename Iterator::reference         reference;
    };

    template <typename T>
    struct iterator_traits<T*>
    {
        typedef EASTL_ITC_NS::random_access_iterator_tag iterator_category;     // To consider: Change this to contiguous_iterator_tag for the case that 
        typedef T                                        value_type;            //              EASTL_ITC_NS is "eastl" instead of "std".
        typedef ptrdiff_t                                difference_type;
        typedef T*                                       pointer;
        typedef T&                                       reference;
    };

    template <typename T>
    struct iterator_traits<const T*>
    {
        typedef EASTL_ITC_NS::random_access_iterator_tag iterator_category;
        typedef T                                        value_type;
        typedef ptrdiff_t                                difference_type;
        typedef const T*                                 pointer;
        typedef const T&                                 reference;
    };





    /// reverse_iterator
    ///
    /// From the C++ standard:
    /// Bidirectional and random access iterators have corresponding reverse 
    /// iterator adaptors that iterate through the data structure in the 
    /// opposite direction. They have the same signatures as the corresponding 
    /// iterators. The fundamental relation between a reverse iterator and its 
    /// corresponding iterator i is established by the identity:
    ///     &*(reverse_iterator(i)) == &*(i - 1).
    /// This mapping is dictated by the fact that while there is always a pointer 
    /// past the end of an array, there might not be a valid pointer before the
    /// beginning of an array.
    ///
    template <typename Iterator>
    class reverse_iterator : public iterator<typename eastl::iterator_traits<Iterator>::iterator_category,
                                             typename eastl::iterator_traits<Iterator>::value_type,
                                             typename eastl::iterator_traits<Iterator>::difference_type,
                                             typename eastl::iterator_traits<Iterator>::pointer,
                                             typename eastl::iterator_traits<Iterator>::reference>
    {
    public:
        typedef Iterator                                                   iterator_type;
        typedef typename eastl::iterator_traits<Iterator>::pointer         pointer;
        typedef typename eastl::iterator_traits<Iterator>::reference       reference;
        typedef typename eastl::iterator_traits<Iterator>::difference_type difference_type;

    protected:
        Iterator mIterator;

    public:
        reverse_iterator()      // It's important that we construct mIterator, because if Iterator  
            : mIterator() { }   // is a pointer, there's a difference between doing it and not.

        explicit reverse_iterator(iterator_type i)
            : mIterator(i) { }

        reverse_iterator(const reverse_iterator& ri)
            : mIterator(ri.mIterator) { }

        template <typename U>
        reverse_iterator(const reverse_iterator<U>& ri)
            : mIterator(ri.base()) { }

        // This operator= isn't in the standard, but the the C++ 
        // library working group has tentatively approved it, as it
        // allows const and non-const reverse_iterators to interoperate.
        template <typename U>
        reverse_iterator<Iterator>& operator=(const reverse_iterator<U>& ri)
            { mIterator = ri.base(); return *this; }

        iterator_type base() const
            { return mIterator; }

        reference operator*() const
        {
            iterator_type i(mIterator);
            return *--i;
        }

        pointer operator->() const
            { return &(operator*()); }

        reverse_iterator& operator++()
            { --mIterator; return *this; }

        reverse_iterator operator++(int)
        {
            reverse_iterator ri(*this);
            --mIterator;
            return ri;
        }

        reverse_iterator& operator--()
            { ++mIterator; return *this; }

        reverse_iterator operator--(int)
        {
            reverse_iterator ri(*this);
            ++mIterator;
            return ri;
        }

        reverse_iterator operator+(difference_type n) const
            { return reverse_iterator(mIterator - n); }

        reverse_iterator& operator+=(difference_type n)
            { mIterator -= n; return *this; }

        reverse_iterator operator-(difference_type n) const
            { return reverse_iterator(mIterator + n); }

        reverse_iterator& operator-=(difference_type n)
            { mIterator += n; return *this; }

        reference operator[](difference_type n) const
            { return mIterator[-n - 1]; } 
    };


    // The C++ library working group has tentatively approved the usage of two
    // template parameters (Iterator1 and Iterator2) in order to allow reverse_iterators
    // and const_reverse iterators to be comparable. This is a similar issue to the 
    // C++ defect report #179 regarding comparison of container iterators and const_iterators.
    template <typename Iterator1, typename Iterator2>
    inline bool
    operator==(const reverse_iterator<Iterator1>& a, const reverse_iterator<Iterator2>& b)
        { return a.base() == b.base(); }


    template <typename Iterator1, typename Iterator2>
    inline bool
    operator<(const reverse_iterator<Iterator1>& a, const reverse_iterator<Iterator2>& b)
        { return a.base() > b.base(); }


    template <typename Iterator1, typename Iterator2>
    inline bool
    operator!=(const reverse_iterator<Iterator1>& a, const reverse_iterator<Iterator2>& b)
        { return a.base() != b.base(); }


    template <typename Iterator1, typename Iterator2>
    inline bool
    operator>(const reverse_iterator<Iterator1>& a, const reverse_iterator<Iterator2>& b)
        { return a.base() < b.base(); }


    template <typename Iterator1, typename Iterator2>
    inline bool
    operator<=(const reverse_iterator<Iterator1>& a, const reverse_iterator<Iterator2>& b)
        { return a.base() >= b.base(); }


    template <typename Iterator1, typename Iterator2>
    inline bool
    operator>=(const reverse_iterator<Iterator1>& a, const reverse_iterator<Iterator2>& b)
        { return a.base() <= b.base(); }


    template <typename Iterator1, typename Iterator2>
    inline typename reverse_iterator<Iterator1>::difference_type
    operator-(const reverse_iterator<Iterator1>& a, const reverse_iterator<Iterator2>& b)
        { return b.base() - a.base(); }


    template <typename Iterator>
    inline reverse_iterator<Iterator>
    operator+(typename reverse_iterator<Iterator>::difference_type n, const reverse_iterator<Iterator>& a)
        { return reverse_iterator<Iterator>(a.base() - n); }







    /// back_insert_iterator
    ///
    /// A back_insert_iterator is simply a class that acts like an iterator but when you 
    /// assign a value to it, it calls push_back on the container with the value.
    ///
    template <typename Container>
    class back_insert_iterator : public iterator<EASTL_ITC_NS::output_iterator_tag, void, void, void, void>
    {
    public:
        typedef Container                           container_type;
        typedef typename Container::const_reference const_reference;

    protected:
        Container& container;

    public:
        explicit back_insert_iterator(Container& x)
            : container(x) { }

        back_insert_iterator& operator=(const_reference value)
            { container.push_back(value); return *this; }

        back_insert_iterator& operator*()
            { return *this; }

        back_insert_iterator& operator++()
            { return *this; } // This is by design.

        back_insert_iterator operator++(int)
            { return *this; } // This is by design.
    };


    /// back_inserter
    ///
    /// Creates an instance of a back_insert_iterator.
    ///
    template <typename Container>
    inline back_insert_iterator<Container>
    back_inserter(Container& x)
        { return back_insert_iterator<Container>(x); }




    /// front_insert_iterator
    ///
    /// A front_insert_iterator is simply a class that acts like an iterator but when you 
    /// assign a value to it, it calls push_front on the container with the value.
    ///
    template <typename Container>
    class front_insert_iterator : public iterator<EASTL_ITC_NS::output_iterator_tag, void, void, void, void>
    {
    public:
        typedef Container                           container_type;
        typedef typename Container::const_reference const_reference;

    protected:
        Container& container;

    public:
        explicit front_insert_iterator(Container& x)
            : container(x) { }

        front_insert_iterator& operator=(const_reference value)
            { container.push_front(value); return *this; }

        front_insert_iterator& operator*()
            { return *this; }

        front_insert_iterator& operator++()
            { return *this; } // This is by design.

        front_insert_iterator operator++(int)
            { return *this; } // This is by design.
    };


    /// front_inserter
    ///
    /// Creates an instance of a front_insert_iterator.
    ///
    template <typename Container>
    inline front_insert_iterator<Container>
    front_inserter(Container& x)
        { return front_insert_iterator<Container>(x); }




    /// insert_iterator
    ///
    /// An insert_iterator is like an iterator except that when you assign a value to it, 
    /// the insert_iterator inserts the value into the container and increments the iterator.
    ///
    /// insert_iterator is an iterator adaptor that functions as an OutputIterator: 
    /// assignment through an insert_iterator inserts an object into a container. 
    /// Specifically, if ii is an insert_iterator, then ii keeps track of a container c and 
    /// an insertion point p; the expression *ii = x performs the insertion c.insert(p, x).
    ///
    /// If you assign through an insert_iterator several times, then you will be inserting 
    /// several elements into the underlying container. In the case of a sequence, they will 
    /// appear at a particular location in the underlying sequence, in the order in which 
    /// they were inserted: one of the arguments to insert_iterator's constructor is an 
    /// iterator p, and the new range will be inserted immediately before p.
    ///
    template <typename Container>
    class insert_iterator : public iterator<EASTL_ITC_NS::output_iterator_tag, void, void, void, void>
    {
    public:
        typedef Container                           container_type;
        typedef typename Container::iterator        iterator_type;
        typedef typename Container::const_reference const_reference;

    protected:
        Container&     container;
        iterator_type  it; 

    public:
        // This assignment operator is defined more to stop compiler warnings (e.g. VC++ C4512)
        // than to be useful. However, it does an insert_iterator to be assigned to another 
        // insert iterator provided that they point to the same container.
        insert_iterator& operator=(const insert_iterator& x)
        {
            EASTL_ASSERT(&x.container == &container);
            it = x.it;
            return *this;
        }

        insert_iterator(Container& x, iterator_type itNew)
            : container(x), it(itNew) {}

        insert_iterator& operator=(const_reference value)
        {
            it = container.insert(it, value);
            ++it;
            return *this;
        }

        insert_iterator& operator*()
            { return *this; }

        insert_iterator& operator++()
            { return *this; } // This is by design.

        insert_iterator& operator++(int)
            { return *this; } // This is by design.

    }; // insert_iterator 


    /// inserter
    ///
    /// Creates an instance of an insert_iterator.
    ///
    template <typename Container, typename Iterator>
    inline eastl::insert_iterator<Container>
    inserter(Container& x, Iterator i)
    {
        typedef typename Container::iterator iterator;
        return eastl::insert_iterator<Container>(x, iterator(i));
    }




    //////////////////////////////////////////////////////////////////////////////////
    /// distance
    ///
    /// Implements the distance() function. There are two versions, one for
    /// random access iterators (e.g. with vector) and one for regular input
    /// iterators (e.g. with list). The former is more efficient.
    ///
    template <typename InputIterator>
    inline typename eastl::iterator_traits<InputIterator>::difference_type
    distance_impl(InputIterator first, InputIterator last, EASTL_ITC_NS::input_iterator_tag)
    {
        typename eastl::iterator_traits<InputIterator>::difference_type n = 0;

        while(first != last)
        {
            ++first;
            ++n;
        }
        return n;
    }

    template <typename RandomAccessIterator>
    inline typename eastl::iterator_traits<RandomAccessIterator>::difference_type
    distance_impl(RandomAccessIterator first, RandomAccessIterator last, EASTL_ITC_NS::random_access_iterator_tag)
    {
        return last - first;
    }

    // Special version defined so that std C++ iterators can be recognized by 
    // this function. Unfortunately, this function treats all foreign iterators
    // as InputIterators and thus can seriously hamper performance in the case
    // of large ranges of bidirectional_iterator_tag iterators.
    //template <typename InputIterator>
    //inline typename eastl::iterator_traits<InputIterator>::difference_type
    //distance_impl(InputIterator first, InputIterator last, ...)
    //{
    //    typename eastl::iterator_traits<InputIterator>::difference_type n = 0;
    //
    //    while(first != last)
    //    {
    //        ++first;
    //        ++n;
    //    }
    //    return n;
    //}

    template <typename InputIterator>
    inline typename eastl::iterator_traits<InputIterator>::difference_type
    distance(InputIterator first, InputIterator last)
    {
        typedef typename eastl::iterator_traits<InputIterator>::iterator_category IC;

        return eastl::distance_impl(first, last, IC());
    }




    //////////////////////////////////////////////////////////////////////////////////
    /// advance
    ///
    /// Implements the advance() function. There are three versions, one for
    /// random access iterators (e.g. with vector), one for bidirectional 
    /// iterators (list) and one for regular input iterators (e.g. with slist). 
    ///
    template <typename InputIterator, typename Distance>
    inline void
    advance_impl(InputIterator& i, Distance n, EASTL_ITC_NS::input_iterator_tag)
    {
        while(n--)
            ++i;
    }

    template <typename BidirectionalIterator, typename Distance>
    inline void
    advance_impl(BidirectionalIterator& i, Distance n, EASTL_ITC_NS::bidirectional_iterator_tag)
    {
        if(n > 0)
        {
            while(n--)
                ++i;
        }
        else
        {
            while(n++)
                --i;
        }
    }

    template <typename RandomAccessIterator, typename Distance>
    inline void
    advance_impl(RandomAccessIterator& i, Distance n, EASTL_ITC_NS::random_access_iterator_tag)
    {
        i += n;
    }

    // Special version defined so that std C++ iterators can be recognized by 
    // this function. Unfortunately, this function treats all foreign iterators
    // as InputIterators and thus can seriously hamper performance in the case
    // of large ranges of bidirectional_iterator_tag iterators.
    //template <typename InputIterator, typename Distance>
    //inline void
    //advance_impl(InputIterator& i, Distance n, ...)
    //{
    //    while(n--)
    //        ++i;
    //}

    template <typename InputIterator, typename Distance>
    inline void
    advance(InputIterator& i, Distance n)
    {
        typedef typename eastl::iterator_traits<InputIterator>::iterator_category IC;

        eastl::advance_impl(i, n, IC());
    }


} // namespace eastl


#if defined(_MSC_VER)
    #pragma warning(pop)
#elif defined(__SNC__)
    #pragma control %pop diag
#endif


#endif // Header include guard





