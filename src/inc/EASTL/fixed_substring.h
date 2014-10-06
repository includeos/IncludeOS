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
// EASTL/fixed_substring.h
//
// Copyright (c) 2005, Electronic Arts. All rights reserved.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_FIXED_SUBSTRING_H
#define EASTL_FIXED_SUBSTRING_H


#include <EASTL/string.h>


namespace eastl
{

    /// fixed_substring
    ///
    /// Implements a string which is a reference to a segment of characters. 
    /// This class is efficient because it allocates no memory and copies no
    /// memory during construction and assignment, but rather refers directly 
    /// to the segment of chracters. A common use of this is to have a 
    /// fixed_substring efficiently refer to a substring within another string.
    ///
    /// You cannot directly resize a fixed_substring (e.g. via resize, insert,
    /// append, erase), but you can assign a different substring to it. 
    /// You can modify the characters within a substring in place.
    /// As of this writing, in the name of being lean and simple it is the 
    /// user's responsibility to not call unsupported resizing functions
    /// such as those listed above. A detailed listing of the functions which
    /// are not supported is given below in the class declaration.
    ///
    /// The c_str function doesn't act as one might hope, as it simply 
    /// returns the pointer to the beginning of the string segment and the
    /// 0-terminator may be beyond the end of the segment. If you want to 
    /// always be able to use c_str as expected, use the fixed string solution 
    /// we describe below.
    ///
    /// Another use of fixed_substring is to provide C++ string-like functionality
    /// with a C character array. This allows you to work on a C character array
    /// as if it were a C++ string as opposed using the C string API. Thus you 
    /// can do this:
    ///
    ///    void DoSomethingForUser(char* timeStr, size_t timeStrCapacity)
    ///    {
    ///        fixed_substring tmp(timeStr, timeStrCapacity);
    ///        tmp  = "hello ";
    ///        tmp += "world";
    ///    }
    ///
    /// Note that this class constructs and assigns from const string pointers
    /// and const string objects, yet this class does not declare its member
    /// data as const. This is a concession in order to allow this implementation
    /// to be simple and lean. It is the user's responsibility to make sure
    /// that strings that should not or can not be modified are either not
    /// used by fixed_substring or are not modified by fixed_substring.
    ///
    /// A more flexible alternative to fixed_substring is fixed_string.
    /// fixed_string has none of the functional limitations that fixed_substring
    /// has and like fixed_substring it doesn't allocate memory. However,
    /// fixed_string makes a *copy* of the source string and uses local
    /// memory to store that copy. Also, fixed_string objects on the stack
    /// are going to have a limit as to their maximum size.
    ///
    /// Notes:
    ///     As of this writing, the string class necessarily reallocates when 
    ///     an insert of self is done into self. As a result, the fixed_substring 
    ///     class doesn't support inserting self into self. 
    ///
    /// Example usage:
    ///     basic_string<char>    str("hello world");
    ///     fixed_substring<char> sub(str, 2, 5);      // sub == "llo w"
    /// 
    template <typename T>
    class fixed_substring : public basic_string<T>
    {
    public:
        typedef basic_string<T>                   base_type;
        typedef fixed_substring<T>                this_type;
        typedef typename base_type::size_type     size_type;
        typedef typename base_type::value_type    value_type;

        using base_type::npos;
        using base_type::mpBegin;
        using base_type::mpEnd;
        using base_type::mpCapacity;
        using base_type::reset;
        using base_type::mAllocator;

    public:
        fixed_substring()
            : base_type()
        {
        }

        fixed_substring(const base_type& x)
            : base_type()
        {
            #if EASTL_NAME_ENABLED
                mAllocator.set_name(x.get_allocator().get_name());
            #endif

            assign(x);
        }

        fixed_substring(const base_type& x, size_type position, size_type n = base_type::npos)
            : base_type()
        {
            #if EASTL_NAME_ENABLED
                mAllocator.set_name(x.get_allocator().get_name());
            #endif

            assign(x, position, n);
        }

        fixed_substring(const value_type* p, size_type n)
            : base_type()
        {
            assign(p, n);
        }

        fixed_substring(const value_type* p)
            : base_type()
        {
             assign(p);
        }

        fixed_substring(const value_type* pBegin, const value_type* pEnd)
            : base_type()
        {
            assign(pBegin, pEnd);
        }

        ~fixed_substring()
        {
            // We need to reset, as otherwise the parent destructor will
            // attempt to free our memory.
            reset();
        }

        this_type& operator=(const base_type& x)
        {
            assign(x);
            return *this;
        }

        this_type& operator=(const value_type* p)
        {
            assign(p);
            return *this;
        }

        this_type& assign(const base_type& x)
        {
            // By design, we need to cast away const-ness here. 
            mpBegin    = const_cast<value_type*>(x.data());
            mpEnd      = mpBegin + x.size();
            mpCapacity = mpEnd;
            return *this;
        }

        this_type& assign(const base_type& x, size_type position, size_type n)
        {
            // By design, we need to cast away const-ness here. 
            mpBegin    = const_cast<value_type*>(x.data()) + position;
            mpEnd      = mpBegin + n;
            mpCapacity = mpEnd;
            return *this;
        }

        this_type& assign(const value_type* p, size_type n)
        {
            // By design, we need to cast away const-ness here. 
            mpBegin    = const_cast<value_type*>(p);
            mpEnd      = mpBegin + n;
            mpCapacity = mpEnd;
            return *this;
        }

        this_type& assign(const value_type* p)
        {
            // By design, we need to cast away const-ness here. 
            mpBegin    = const_cast<value_type*>(p);
            mpEnd      = mpBegin + CharStrlen(p);
            mpCapacity = mpEnd;
            return *this;
        }

        this_type& assign(const value_type* pBegin, const value_type* pEnd)
        {
            // By design, we need to cast away const-ness here. 
            mpBegin    = const_cast<value_type*>(pBegin);
            mpEnd      = const_cast<value_type*>(pEnd);
            mpCapacity = mpEnd;
            return *this;
        }


        // Partially supported functionality
        //
        // When using fixed_substring on a character sequence that is within another
        // string, the following functions may do one of two things:
        //     1 Attempt to reallocate
        //     2 Write a 0 char at the end of the fixed_substring
        //
        // Item #1 will result in a crash, due to the attempt by the underlying 
        // string class to free the substring memory. Item #2 will result in a 0 
        // char being written to the character array. Item #2 may or may not be 
        // a problem, depending on how you use fixed_substring. Thus the following
        // functions should be used carefully.
        //
        // basic_string&  operator=(const basic_string& x);
        // basic_string&  operator=(value_type c);
        // void           resize(size_type n, value_type c);
        // void           resize(size_type n);
        // void           reserve(size_type = 0);
        // void           set_capacity(size_type n);
        // void           clear();
        // basic_string&  operator+=(const basic_string& x);
        // basic_string&  operator+=(const value_type* p);
        // basic_string&  operator+=(value_type c);
        // basic_string&  append(const basic_string& x);
        // basic_string&  append(const basic_string& x, size_type position, size_type n);
        // basic_string&  append(const value_type* p, size_type n);
        // basic_string&  append(const value_type* p);
        // basic_string&  append(size_type n);
        // basic_string&  append(size_type n, value_type c);
        // basic_string&  append(const value_type* pBegin, const value_type* pEnd);
        // basic_string&  append_sprintf_va_list(const value_type* pFormat, va_list arguments);
        // basic_string&  append_sprintf(const value_type* pFormat, ...);
        // void           push_back(value_type c);
        // void           pop_back();
        // basic_string&  assign(const value_type* p, size_type n);
        // basic_string&  assign(size_type n, value_type c);
        // basic_string&  insert(size_type position, const basic_string& x);
        // basic_string&  insert(size_type position, const basic_string& x, size_type beg, size_type n);
        // basic_string&  insert(size_type position, const value_type* p, size_type n);
        // basic_string&  insert(size_type position, const value_type* p);
        // basic_string&  insert(size_type position, size_type n, value_type c);
        // iterator       insert(iterator p, value_type c);
        // void           insert(iterator p, size_type n, value_type c);
        // void           insert(iterator p, const value_type* pBegin, const value_type* pEnd);
        // basic_string&  erase(size_type position = 0, size_type n = npos);
        // iterator       erase(iterator p);
        // iterator       erase(iterator pBegin, iterator pEnd);
        // void           swap(basic_string& x);
        // basic_string&  sprintf_va_list(const value_type* pFormat, va_list arguments);
        // basic_string&  sprintf(const value_type* pFormat, ...);


    }; // fixed_substring


} // namespace eastl



#endif // Header include guard












