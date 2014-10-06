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
// EASTL/utility.h
// Written and maintained by Paul Pedriana - 2005.
///////////////////////////////////////////////////////////////////////////////



#ifndef EASTL_UTILITY_H
#define EASTL_UTILITY_H


#include <EASTL/internal/config.h>
#include <utility.hpp>

#ifdef _MSC_VER
    #pragma warning(push)           // VC++ generates a bogus warning that you cannot code away.
    #pragma warning(disable: 4619)  // There is no warning number 'number'.
    #pragma warning(disable: 4217)  // Member template functions cannot be used for copy-assignment or copy-construction.
    #pragma warning(disable: 4512)  // 'class' : assignment operator could not be generated.  // This disabling would best be put elsewhere.
#endif


namespace eastl
{

    ///////////////////////////////////////////////////////////////////////
    /// rel_ops
    ///
    /// rel_ops allow the automatic generation of operators !=, >, <=, >= from
    /// just operators == and <. These are intentionally in the rel_ops namespace
    /// so that they don't conflict with other similar operators. To use these 
    /// operators, add "using namespace std::rel_ops;" to an appropriate place in 
    /// your code, usually right in the function that you need them to work.
    /// In fact, you will very likely have collision problems if you put such
    /// using statements anywhere other than in the .cpp file like so and may 
    /// also have collisions when you do, as the using statement will affect all
    /// code in the module. You need to be careful about use of rel_ops.
    ///
    namespace rel_ops
    {
        template <typename T>
        inline bool operator!=(const T& x, const T& y)
            { return !(x == y); }

        template <typename T>
        inline bool operator>(const T& x, const T& y)
            { return (y < x); }

        template <typename T>
        inline bool operator<=(const T& x, const T& y)
            { return !(y < x); }

        template <typename T>
        inline bool operator>=(const T& x, const T& y)
            { return !(x < y); }
    }



    ///////////////////////////////////////////////////////////////////////
    /// pair
    ///
    /// Implements a simple pair, just like the C++ std::pair.
    ///
    template <typename T1, typename T2>
    struct pair
    {
        typedef T1 first_type;
        typedef T2 second_type;

        T1 first;
        T2 second;

        pair();
        pair(const T1& x);
        pair(const T1& x, const T2& y);

        template <typename U, typename V>
        pair(const pair<U, V>& p);

        // pair(const pair& p);              // Not necessary, as default version is OK.
        // pair& operator=(const pair& p);   // Not necessary, as default version is OK.
    };




    /// use_self
    ///
    /// operator()(x) simply returns x. Used in sets, as opposed to maps.
    /// This is a template policy implementation; it is an alternative to 
    /// the use_first template implementation.
    ///
    /// The existance of use_self may seem odd, given that it does nothing,
    /// but these kinds of things are useful, virtually required, for optimal 
    /// generic programming.
    ///
    template <typename T>
    struct use_self             // : public unary_function<T, T> // Perhaps we want to make it a subclass of unary_function.
    {
        typedef T result_type;

        const T& operator()(const T& x) const
            { return x; }
    };

    /// use_first
    ///
    /// operator()(x) simply returns x.first. Used in maps, as opposed to sets.
    /// This is a template policy implementation; it is an alternative to 
    /// the use_self template implementation. This is the same thing as the
    /// SGI SGL select1st utility.
    ///
    template <typename Pair>
    struct use_first            // : public unary_function<Pair, typename Pair::first_type> // Perhaps we want to make it a subclass of unary_function.
    {
        typedef typename Pair::first_type result_type;

        const result_type& operator()(const Pair& x) const
            { return x.first; }
    };

    /// use_second
    ///
    /// operator()(x) simply returns x.second. 
    /// This is the same thing as the SGI SGL select2nd utility
    ///
    template <typename Pair>
    struct use_second           // : public unary_function<Pair, typename Pair::second_type> // Perhaps we want to make it a subclass of unary_function.
    {
        typedef typename Pair::second_type result_type;

        const result_type& operator()(const Pair& x) const
            { return x.second; }
    };





    ///////////////////////////////////////////////////////////////////////
    // pair
    ///////////////////////////////////////////////////////////////////////

    template <typename T1, typename T2>
    inline pair<T1, T2>::pair()
        : first(), second()
    {
        // Empty
    }


    template <typename T1, typename T2>
    inline pair<T1, T2>::pair(const T1& x)
        : first(x), second()
    {
        // Empty
    }


    template <typename T1, typename T2>
    inline pair<T1, T2>::pair(const T1& x, const T2& y)
        : first(x), second(y)
    {
        // Empty
    }


    template <typename T1, typename T2>
    template <typename U, typename V>
    inline pair<T1, T2>::pair(const pair<U, V>& p)
        : first(p.first), second(p.second)
    {
        // Empty
    }




    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template <typename T1, typename T2>
    inline bool operator==(const pair<T1, T2>& a, const pair<T1, T2>& b)
    {
        return ((a.first == b.first) && (a.second == b.second));
    }


    template <typename T1, typename T2>
    inline bool operator<(const pair<T1, T2>& a, const pair<T1, T2>& b)
    {
        // Note that we use only operator < in this expression. Otherwise we could
        // use the simpler: return (a.m1 == b.m1) ? (a.m2 < b.m2) : (a.m1 < b.m1);
        // The user can write a specialization for this operator to get around this
        // in cases where the highest performance is required.
        return ((a.first < b.first) || (!(b.first < a.first) && (a.second < b.second)));
    }


    template <typename T1, typename T2>
    inline bool operator!=(const pair<T1, T2>& a, const pair<T1, T2>& b)
    {
        return !(a == b);
    }


    template <typename T1, typename T2>
    inline bool operator>(const pair<T1, T2>& a, const pair<T1, T2>& b)
    {
        return b < a;
    }


    template <typename T1, typename T2>
    inline bool operator>=(const pair<T1, T2>& a, const pair<T1, T2>& b)
    {
        return !(a < b);
    }


    template <typename T1, typename T2>
    inline bool operator<=(const pair<T1, T2>& a, const pair<T1, T2>& b)
    {
        return !(b < a);
    }




    ///////////////////////////////////////////////////////////////////////
    /// make_pair / make_pair_ref
    ///
    /// make_pair is the same as std::make_pair specified by the C++ standard.
    /// If you look at the C++ standard, you'll see that it specifies T& instead of T.
    /// However, it has been determined that the C++ standard is incorrect and has 
    /// flagged it as a defect (http://www.open-std.org/jtc1/sc22/wg21/docs/lwg-defects.html#181).
    /// In case you feel that you want a more efficient version that uses references,
    /// we provide the make_pair_ref function below.
    /// 
    /// Note: You don't need to use make_pair in order to make a pair. The following
    /// code is equivalent, and the latter avoids one more level of inlining:
    ///     return make_pair(charPtr, charPtr);
    ///     return pair<char*, char*>(charPtr, charPtr);
    ///
    template <typename T1, typename T2>
    inline pair<T1, T2> make_pair(T1 a, T2 b)
    {
        return pair<T1, T2>(a, b);
    }


    template <typename T1, typename T2>
    inline pair<T1, T2> make_pair_ref(const T1& a, const T2& b)
    {
        return pair<T1, T2>(a, b);
    }


} // namespace eastl


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard















