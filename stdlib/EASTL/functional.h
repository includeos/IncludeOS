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
// EASTL/functional.h
// Written and maintained by Paul Pedriana - 2005
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_FUNCTIONAL_H
#define EASTL_FUNCTIONAL_H


#include <EASTL/internal/config.h>
#include <EASTL/type_traits.h>


namespace eastl
{

    ///////////////////////////////////////////////////////////////////////
    // Primary C++ functions
    ///////////////////////////////////////////////////////////////////////

    template <typename Argument, typename Result>
    struct unary_function
    {
        typedef Argument argument_type;
        typedef Result   result_type;
    };


    template <typename Argument1, typename Argument2, typename Result>
    struct binary_function
    {
        typedef Argument1 first_argument_type;
        typedef Argument2 second_argument_type;
        typedef Result    result_type;
    };


    template <typename T>
    struct plus : public binary_function<T, T, T>
    {
        T operator()(const T& a, const T& b) const
            { return a + b; }
    };

    template <typename T>
    struct minus : public binary_function<T, T, T>
    {
        T operator()(const T& a, const T& b) const
            { return a - b; }
    };

    template <typename T>
    struct multiplies : public binary_function<T, T, T>
    {
        T operator()(const T& a, const T& b) const
            { return a * b; }
    };

    template <typename T>
    struct divides : public binary_function<T, T, T>
    {
        T operator()(const T& a, const T& b) const
            { return a / b; }
    };

    template <typename T>
    struct modulus : public binary_function<T, T, T>
    {
        T operator()(const T& a, const T& b) const
            { return a % b; }
    };

    template <typename T>
    struct negate : public unary_function<T, T>
    {
        T operator()(const T& a) const
            { return -a; }
    };

    template <typename T>
    struct equal_to : public binary_function<T, T, bool>
    {
        bool operator()(const T& a, const T& b) const
            { return a == b; }
    };

    template <typename T, typename Compare>
    bool validate_equal_to(const T& a, const T& b, Compare compare)
    {
        return compare(a, b) == compare(b, a);
    }

    template <typename T>
    struct not_equal_to : public binary_function<T, T, bool>
    {
        bool operator()(const T& a, const T& b) const
            { return a != b; }
    };

    template <typename T, typename Compare>
    bool validate_not_equal_to(const T& a, const T& b, Compare compare)
    {
        return compare(a, b) == compare(b, a); // We want the not equal comparison results to be equal.
    }

    /// str_equal_to
    ///
    /// Compares two 0-terminated string types.
    /// The T types are expected to be iterators or act like iterators.
    ///
    /// Example usage:
    ///     hash_set<const char*, hash<const char*>, str_equal_to<const char*> > stringHashSet;
    ///
    /// Note:
    /// You couldn't use str_equal_to like this:
    ///     bool result = equal("hi", "hi" + 2, "ho", str_equal_to<const char*>());
    /// This is because equal tests an array of something, with each element by
    /// the comparison function. But str_equal_to tests an array of something itself.
    ///
    template <typename T>
    struct str_equal_to : public binary_function<T, T, bool>
    {
        bool operator()(T a, T b) const
        {
            while(*a && (*a == *b))
            {
                ++a;
                ++b;
            }
            return (*a == *b);
        }
    };

    template <typename T>
    struct greater : public binary_function<T, T, bool>
    {
        bool operator()(const T& a, const T& b) const
            { return a > b; }
    };

    template <typename T, typename Compare>
    bool validate_greater(const T& a, const T& b, Compare compare)
    {
        return !compare(a, b) || !compare(b, a); // If (a > b), then !(b > a)
    }

    template <typename T>
    struct less : public binary_function<T, T, bool>
    {
        bool operator()(const T& a, const T& b) const
            { return a < b; }
    };

    template <typename T, typename Compare>
    bool validate_less(const T& a, const T& b, Compare compare)
    {
        return !compare(a, b) || !compare(b, a); // If (a < b), then !(b < a)
    }

    template <typename T>
    struct greater_equal : public binary_function<T, T, bool>
    {
        bool operator()(const T& a, const T& b) const
        { return a >= b; }
    };

    template <typename T, typename Compare>
    bool validate_greater_equal(const T& a, const T& b, Compare compare)
    {
        return !compare(a, b) || !compare(b, a); // If (a >= b), then !(b >= a)
    }

    template <typename T>
    struct less_equal : public binary_function<T, T, bool>
    {
        bool operator()(const T& a, const T& b) const
            { return a <= b; }
    };

    template <typename T, typename Compare>
    bool validate_less_equal(const T& a, const T& b, Compare compare)
    {
        return !compare(a, b) || !compare(b, a); // If (a <= b), then !(b <= a)
    }

    template <typename T>
    struct logical_and : public binary_function<T, T, bool>
    {
        bool operator()(const T& a, const T& b) const
            { return a && b; }
    };

    template <typename T>
    struct logical_or : public binary_function<T, T, bool>
    {
        bool operator()(const T& a, const T& b) const
            { return a || b; }
    };

    template <typename T>
    struct logical_not : public unary_function<T, bool>
    {
        bool operator()(const T& a) const
            { return !a; }
    };



    ///////////////////////////////////////////////////////////////////////
    // Dual type functions
    ///////////////////////////////////////////////////////////////////////

    template <typename T, typename U>
    struct equal_to_2 : public binary_function<T, U, bool>
    {
        bool operator()(const T& a, const U& b) const
            { return a == b; }
    };

    template <typename T, typename U>
    struct not_equal_to_2 : public binary_function<T, U, bool>
    {
        bool operator()(const T& a, const U& b) const
            { return a != b; }
    };

    template <typename T, typename U>
    struct less_2 : public binary_function<T, U, bool>
    {
        bool operator()(const T& a, const U& b) const
            { return a < b; }
    };




    /// unary_negate
    ///
    template <typename Predicate>
    class unary_negate : public unary_function<typename Predicate::argument_type, bool>
    {
        protected:
            Predicate mPredicate;
        public:
            explicit unary_negate(const Predicate& a)
                : mPredicate(a) {}
            bool operator()(const typename Predicate::argument_type& a) const
                { return !mPredicate(a); }
    };

    template <typename Predicate>
    inline unary_negate<Predicate> not1(const Predicate& predicate)
        { return unary_negate<Predicate>(predicate); }



    /// binary_negate
    ///
    template <typename Predicate>
    class binary_negate : public binary_function<typename Predicate::first_argument_type, typename Predicate::second_argument_type, bool>
    {
        protected:
            Predicate mPredicate;
        public:
            explicit binary_negate(const Predicate& a)
                : mPredicate(a) { }
            bool operator()(const typename Predicate::first_argument_type& a, const typename Predicate::second_argument_type& b) const
                { return !mPredicate(a, b); }
    };

    template <typename Predicate>
    inline binary_negate<Predicate> not2(const Predicate& predicate)
        { return binary_negate<Predicate>(predicate); }




    ///////////////////////////////////////////////////////////////////////
    // bind
    ///////////////////////////////////////////////////////////////////////

    /// bind1st
    ///
    template <typename Operation>
    class binder1st : public unary_function<typename Operation::second_argument_type, typename Operation::result_type>
    {
        protected:
            typename Operation::first_argument_type value;
            Operation op;

        public:
            binder1st(const Operation& x, const typename Operation::first_argument_type& y)
                : value(y), op(x) { }

            typename Operation::result_type operator()(const typename Operation::second_argument_type& x) const
                { return op(value, x); }

            typename Operation::result_type operator()(typename Operation::second_argument_type& x) const
                { return op(value, x); }
    };


    template <typename Operation, typename T>
    inline binder1st<Operation> bind1st(const Operation& op, const T& x)
    {
        typedef typename Operation::first_argument_type value;
        return binder1st<Operation>(op, value(x));
    }


    /// bind2nd
    ///
    template <typename Operation>
    class binder2nd : public unary_function<typename Operation::first_argument_type, typename Operation::result_type>
    {
        protected:
            Operation op;
            typename Operation::second_argument_type value;

        public:
            binder2nd(const Operation& x, const typename Operation::second_argument_type& y)
                : op(x), value(y) { }

            typename Operation::result_type operator()(const typename Operation::first_argument_type& x) const
                { return op(x, value); }

            typename Operation::result_type operator()(typename Operation::first_argument_type& x) const
                { return op(x, value); }
    };


    template <typename Operation, typename T>
    inline binder2nd<Operation> bind2nd(const Operation& op, const T& x)
    {
        typedef typename Operation::second_argument_type value;
        return binder2nd<Operation>(op, value(x));
    }




    ///////////////////////////////////////////////////////////////////////
    // pointer_to_unary_function
    ///////////////////////////////////////////////////////////////////////

    /// pointer_to_unary_function
    ///
    /// This is an adapter template which converts a pointer to a standalone
    /// function to a function object. This allows standalone functions to 
    /// work in many cases where the system requires a function object.
    ///
    /// Example usage:
    ///     ptrdiff_t Rand(ptrdiff_t n) { return rand() % n; } // Note: The C rand function is poor and slow.
    ///     pointer_to_unary_function<ptrdiff_t, ptrdiff_t> randInstance(Rand);
    ///     random_shuffle(pArrayBegin, pArrayEnd, randInstance);
    ///
    template <typename Arg, typename Result>
    class pointer_to_unary_function : public unary_function<Arg, Result>
    {
    protected:
        Result (*mpFunction)(Arg);

    public:
        pointer_to_unary_function()
            { }

        explicit pointer_to_unary_function(Result (*pFunction)(Arg))
            : mpFunction(pFunction) { }

        Result operator()(Arg x) const
            { return mpFunction(x); } 
    };


    /// ptr_fun
    ///
    /// This ptr_fun is simply shorthand for usage of pointer_to_unary_function.
    ///
    /// Example usage (actually, you don't need to use ptr_fun here, but it works anyway):
    ///    int factorial(int x) { return (x > 1) ? (x * factorial(x - 1)) : x; }
    ///    transform(pIntArrayBegin, pIntArrayEnd, pIntArrayBegin, ptr_fun(factorial));
    ///
    template <typename Arg, typename Result>
    inline pointer_to_unary_function<Arg, Result> ptr_fun(Result (*pFunction)(Arg))
        { return pointer_to_unary_function<Arg, Result>(pFunction); }





    ///////////////////////////////////////////////////////////////////////
    // pointer_to_binary_function
    ///////////////////////////////////////////////////////////////////////

    /// pointer_to_binary_function
    ///
    /// This is an adapter template which converts a pointer to a standalone
    /// function to a function object. This allows standalone functions to 
    /// work in many cases where the system requires a function object.
    ///
    template <typename Arg1, typename Arg2, typename Result>
    class pointer_to_binary_function : public binary_function<Arg1, Arg2, Result>
    {
    protected:
        Result (*mpFunction)(Arg1, Arg2);

    public:
        pointer_to_binary_function()
            { }

        explicit pointer_to_binary_function(Result (*pFunction)(Arg1, Arg2))
            : mpFunction(pFunction) {}

        Result operator()(Arg1 x, Arg2 y) const
            { return mpFunction(x, y); }
    };


    /// This ptr_fun is simply shorthand for usage of pointer_to_binary_function.
    ///
    /// Example usage (actually, you don't need to use ptr_fun here, but it works anyway):
    ///    int multiply(int x, int y) { return x * y; }
    ///    transform(pIntArray1Begin, pIntArray1End, pIntArray2Begin, pIntArray1Begin, ptr_fun(multiply));
    ///
    template <typename Arg1, typename Arg2, typename Result>
    inline pointer_to_binary_function<Arg1, Arg2, Result> ptr_fun(Result (*pFunction)(Arg1, Arg2))
        { return pointer_to_binary_function<Arg1, Arg2, Result>(pFunction); }






    ///////////////////////////////////////////////////////////////////////
    // mem_fun
    // mem_fun1
    //
    // Note that mem_fun calls member functions via *pointers* to classes 
    // and not instances of classes. mem_fun_ref is for calling functions
    // via instances of classes or references to classes.
    //
    ///////////////////////////////////////////////////////////////////////

    /// mem_fun_t
    ///
    /// Member function with no arguments.
    ///
    template <typename Result, typename T> 
    class mem_fun_t : public unary_function<T*, Result>
    {
    public:
        typedef Result (T::*MemberFunction)();

        EA_FORCE_INLINE explicit mem_fun_t(MemberFunction pMemberFunction)
            : mpMemberFunction(pMemberFunction)
        {
            // Empty
        }

        EA_FORCE_INLINE Result operator()(T* pT) const
        {
            return (pT->*mpMemberFunction)();
        }

    protected:
        MemberFunction mpMemberFunction;
    };


    /// mem_fun1_t
    ///
    /// Member function with one argument.
    ///
    template <typename Result, typename T, typename Argument>
    class mem_fun1_t : public binary_function<T*, Argument, Result>
    {
    public:
        typedef Result (T::*MemberFunction)(Argument);

        EA_FORCE_INLINE explicit mem_fun1_t(MemberFunction pMemberFunction)
            : mpMemberFunction(pMemberFunction)
        {
            // Empty
        }

        EA_FORCE_INLINE Result operator()(T* pT, Argument arg) const
        {
            return (pT->*mpMemberFunction)(arg);
        }

    protected:
        MemberFunction mpMemberFunction;
    };


    /// const_mem_fun_t
    ///
    /// Const member function with no arguments.
    /// Note that we inherit from unary_function<const T*, Result>
    /// instead of what the C++ standard specifies: unary_function<T*, Result>.
    /// The C++ standard is in error and this has been recognized by the defect group.
    ///
    template <typename Result, typename T>
    class const_mem_fun_t : public unary_function<const T*, Result>
    {
    public:
        typedef Result (T::*MemberFunction)() const;

        EA_FORCE_INLINE explicit const_mem_fun_t(MemberFunction pMemberFunction)
            : mpMemberFunction(pMemberFunction)
        {
            // Empty
        }

        EA_FORCE_INLINE Result operator()(const T* pT) const
        {
            return (pT->*mpMemberFunction)();
        }

    protected:
        MemberFunction mpMemberFunction;
    };


    /// const_mem_fun1_t
    ///
    /// Const member function with one argument.
    /// Note that we inherit from unary_function<const T*, Result>
    /// instead of what the C++ standard specifies: unary_function<T*, Result>.
    /// The C++ standard is in error and this has been recognized by the defect group.
    ///
    template <typename Result, typename T, typename Argument>
    class const_mem_fun1_t : public binary_function<const T*, Argument, Result>
    {
    public:
        typedef Result (T::*MemberFunction)(Argument) const;

        EA_FORCE_INLINE explicit const_mem_fun1_t(MemberFunction pMemberFunction)
            : mpMemberFunction(pMemberFunction)
        {
            // Empty
        }

        EA_FORCE_INLINE Result operator()(const T* pT, Argument arg) const
        {
            return (pT->*mpMemberFunction)(arg);
        }

    protected:
        MemberFunction mpMemberFunction;
    };


    /// mem_fun
    ///
    /// This is the high level interface to the mem_fun_t family.
    ///
    /// Example usage:
    ///    struct TestClass { void print() { puts("hello"); } }
    ///    TestClass* pTestClassArray[3] = { ... };
    ///    for_each(pTestClassArray, pTestClassArray + 3, &TestClass::print);
    ///
    template <typename Result, typename T>
    EA_FORCE_INLINE mem_fun_t<Result, T>
    mem_fun(Result (T::*MemberFunction)())
    {
        return eastl::mem_fun_t<Result, T>(MemberFunction);
    }

    template <typename Result, typename T, typename Argument>
    EA_FORCE_INLINE mem_fun1_t<Result, T, Argument>
    mem_fun(Result (T::*MemberFunction)(Argument))
    {
        return eastl::mem_fun1_t<Result, T, Argument>(MemberFunction);
    }

    template <typename Result, typename T>
    EA_FORCE_INLINE const_mem_fun_t<Result, T>
    mem_fun(Result (T::*MemberFunction)() const)
    {
        return eastl::const_mem_fun_t<Result, T>(MemberFunction);
    }

    template <typename Result, typename T, typename Argument>
    EA_FORCE_INLINE const_mem_fun1_t<Result, T, Argument>
    mem_fun(Result (T::*MemberFunction)(Argument) const)
    {
        return eastl::const_mem_fun1_t<Result, T, Argument>(MemberFunction);
    }





    ///////////////////////////////////////////////////////////////////////
    // mem_fun_ref
    // mem_fun1_ref
    //
    ///////////////////////////////////////////////////////////////////////

    /// mem_fun_ref_t
    ///
    template <typename Result, typename T>
    class mem_fun_ref_t : public unary_function<T, Result>
    {
    public:
        typedef Result (T::*MemberFunction)();

        EA_FORCE_INLINE explicit mem_fun_ref_t(MemberFunction pMemberFunction)
            : mpMemberFunction(pMemberFunction)
        {
            // Empty
        }

        EA_FORCE_INLINE Result operator()(T& t) const
        {
            return (t.*mpMemberFunction)();
        }

    protected:
        MemberFunction mpMemberFunction;
    };


    /// mem_fun1_ref_t
    ///
    template <typename Result, typename T, typename Argument>
    class mem_fun1_ref_t : public binary_function<T, Argument, Result>
    {
    public:
        typedef Result (T::*MemberFunction)(Argument);

        EA_FORCE_INLINE explicit mem_fun1_ref_t(MemberFunction pMemberFunction)
            : mpMemberFunction(pMemberFunction)
        {
            // Empty
        }

        EA_FORCE_INLINE Result operator()(T& t, Argument arg) const
        {
            return (t.*mpMemberFunction)(arg);
        }

    protected:
        MemberFunction mpMemberFunction;
    };


    /// const_mem_fun_ref_t
    ///
    template <typename Result, typename T>
    class const_mem_fun_ref_t : public unary_function<T, Result>
    {
    public:
        typedef Result (T::*MemberFunction)() const;

        EA_FORCE_INLINE explicit const_mem_fun_ref_t(MemberFunction pMemberFunction)
            : mpMemberFunction(pMemberFunction)
        {
            // Empty
        }

        EA_FORCE_INLINE Result operator()(const T& t) const
        {
            return (t.*mpMemberFunction)();
        }

    protected:
        MemberFunction mpMemberFunction;
    };


    /// const_mem_fun1_ref_t
    ///
    template <typename Result, typename T, typename Argument>
    class const_mem_fun1_ref_t : public binary_function<T, Argument, Result>
    {
    public:
        typedef Result (T::*MemberFunction)(Argument) const;

        EA_FORCE_INLINE explicit const_mem_fun1_ref_t(MemberFunction pMemberFunction)
            : mpMemberFunction(pMemberFunction)
        {
            // Empty
        }

        EA_FORCE_INLINE Result operator()(const T& t, Argument arg) const
        {
            return (t.*mpMemberFunction)(arg);
        }

    protected:
        MemberFunction mpMemberFunction;
    };


    /// mem_fun_ref
    /// Example usage:
    ///    struct TestClass { void print() { puts("hello"); } }
    ///    TestClass testClassArray[3];
    ///    for_each(testClassArray, testClassArray + 3, &TestClass::print);
    ///
    template <typename Result, typename T>
    EA_FORCE_INLINE mem_fun_ref_t<Result, T>
    mem_fun_ref(Result (T::*MemberFunction)())
    {
        return eastl::mem_fun_ref_t<Result, T>(MemberFunction);
    }

    template <typename Result, typename T, typename Argument>
    EA_FORCE_INLINE mem_fun1_ref_t<Result, T, Argument>
    mem_fun_ref(Result (T::*MemberFunction)(Argument))
    {
        return eastl::mem_fun1_ref_t<Result, T, Argument>(MemberFunction);
    }

    template <typename Result, typename T>
    EA_FORCE_INLINE const_mem_fun_ref_t<Result, T>
    mem_fun_ref(Result (T::*MemberFunction)() const)
    {
        return eastl::const_mem_fun_ref_t<Result, T>(MemberFunction);
    }

    template <typename Result, typename T, typename Argument>
    EA_FORCE_INLINE const_mem_fun1_ref_t<Result, T, Argument>
    mem_fun_ref(Result (T::*MemberFunction)(Argument) const)
    {
        return eastl::const_mem_fun1_ref_t<Result, T, Argument>(MemberFunction);
    }




    ///////////////////////////////////////////////////////////////////////
    // hash
    ///////////////////////////////////////////////////////////////////////

    template <typename T> struct hash;

    template <typename T> struct hash<T*> // Note that we use the pointer as-is and don't divide by sizeof(T*). This is because the table is of a prime size and this division doesn't benefit distribution.
        { size_t operator()(T* p) const { return size_t(uintptr_t(p)); } };

    template <> struct hash<bool>
        { size_t operator()(bool val) const { return static_cast<size_t>(val); } };

    template <> struct hash<char>
        { size_t operator()(char val) const { return static_cast<size_t>(val); } };

    template <> struct hash<signed char>
        { size_t operator()(signed char val) const { return static_cast<size_t>(val); } };

    template <> struct hash<unsigned char>
        { size_t operator()(unsigned char val) const { return static_cast<size_t>(val); } };

    #ifndef EA_WCHAR_T_NON_NATIVE // If wchar_t is a native type instead of simply a define to an existing type...
        template <> struct hash<wchar_t>
            { size_t operator()(wchar_t val) const { return static_cast<size_t>(val); } };
    #endif

    template <> struct hash<signed short>
        { size_t operator()(short val) const { return static_cast<size_t>(val); } };

    template <> struct hash<unsigned short>
        { size_t operator()(unsigned short val) const { return static_cast<size_t>(val); } };

    template <> struct hash<signed int>
        { size_t operator()(signed int val) const { return static_cast<size_t>(val); } };

    template <> struct hash<unsigned int>
        { size_t operator()(unsigned int val) const { return static_cast<size_t>(val); } };

    template <> struct hash<signed long>
        { size_t operator()(signed long val) const { return static_cast<size_t>(val); } };

    template <> struct hash<unsigned long>
        { size_t operator()(unsigned long val) const { return static_cast<size_t>(val); } };

    template <> struct hash<signed long long>
        { size_t operator()(signed long long val) const { return static_cast<size_t>(val); } };

    template <> struct hash<unsigned long long>
        { size_t operator()(unsigned long long val) const { return static_cast<size_t>(val); } };

    template <> struct hash<float>
        { size_t operator()(float val) const { return static_cast<size_t>(val); } };

    template <> struct hash<double>
        { size_t operator()(double val) const { return static_cast<size_t>(val); } };

    template <> struct hash<long double>
        { size_t operator()(long double val) const { return static_cast<size_t>(val); } };


    ///////////////////////////////////////////////////////////////////////////
    // string hashes
    //
    // Note that our string hashes here intentionally are slow for long strings.
    // The reasoning for this is so:
    //    - The large majority of hashed strings are only a few bytes long.
    //    - The hash function is significantly more efficient if it can make this assumption.
    //    - The user is welcome to make a custom hash for those uncommon cases where
    //      long strings need to be hashed. Indeed, the user can probably make a 
    //      special hash customized for such strings that's better than what we provide.
    ///////////////////////////////////////////////////////////////////////////

    template <> struct hash<char8_t*>
    {
        size_t operator()(const char8_t* p) const
        {
            size_t c, result = 2166136261U;  // FNV1 hash. Perhaps the best string hash.
            while((c = (uint8_t)*p++) != 0)  // Using '!=' disables compiler warnings.
                result = (result * 16777619) ^ c;
            return (size_t)result;
        }
    };

    template <> struct hash<const char8_t*>
    {
        size_t operator()(const char8_t* p) const
        {
            size_t c, result = 2166136261U;
            while((c = (uint8_t)*p++) != 0) // cast to unsigned 8 bit.
                result = (result * 16777619) ^ c;
            return (size_t)result;
        }
    };

    template <> struct hash<char16_t*>
    {
        size_t operator()(const char16_t* p) const
        {
            size_t c, result = 2166136261U;
            while((c = (uint16_t)*p++) != 0) // cast to unsigned 16 bit.
                result = (result * 16777619) ^ c;
            return (size_t)result;
        }
    };

    template <> struct hash<const char16_t*>
    {
        size_t operator()(const char16_t* p) const
        {
            size_t c, result = 2166136261U;
            while((c = (uint16_t)*p++) != 0) // cast to unsigned 16 bit.
                result = (result * 16777619) ^ c;
            return (size_t)result;
        }
    };

    template <> struct hash<char32_t*>
    {
        size_t operator()(const char32_t* p) const
        {
            size_t c, result = 2166136261U;
            while((c = (uint32_t)*p++) != 0) // cast to unsigned 32 bit.
                result = (result * 16777619) ^ c;
            return (size_t)result;
        }
    };

    template <> struct hash<const char32_t*>
    {
        size_t operator()(const char32_t* p) const
        {
            size_t c, result = 2166136261U;
            while((c = (uint32_t)*p++) != 0) // cast to unsigned 32 bit.
                result = (result * 16777619) ^ c;
            return (size_t)result;
        }
    };

    /// string_hash
    ///
    /// Defines a generic string hash for an arbitrary EASTL basic_string container.
    ///
    /// Example usage:
    ///    eastl::hash_set<MyString, eastl::string_hash<MyString> > hashSet;
    ///
    template <typename String>
    struct string_hash
    {
        typedef String                                         string_type;
        typedef typename String::value_type                    value_type;
        typedef typename eastl::add_unsigned<value_type>::type unsigned_value_type;

        size_t operator()(const string_type& s) const
        {
            const unsigned_value_type* p = (const unsigned_value_type*)s.c_str();
            size_t c, result = 2166136261U;
            while((c = *p++) != 0)
                result = (result * 16777619) ^ c;
            return (size_t)result;
        }
    };


} // namespace eastl


#endif // Header include guard







