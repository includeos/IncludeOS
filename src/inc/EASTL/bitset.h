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
// EASTL/bitset.h
//
// Written and maintained by Paul Pedriana - 2005.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements a bitset much like the C++ std::bitset class. 
// The primary distinctions between this list and std::bitset are:
//    - bitset is more efficient than some other std::bitset implementations,
//    - bitset is savvy to an environment that doesn't have exception handling,
//      as is sometimes the case with console or embedded environments.
//    - bitset is savvy to environments in which 'unsigned long' is not the 
//      most efficient integral data type. std::bitset implementations use
//      unsigned long, even if it is an inefficient integer type.
//    - bitset removes as much function calls as practical, in order to allow
//      debug builds to run closer in speed and code footprint to release builds.
//    - bitset doesn't support string functionality. We can add this if 
//      it is deemed useful.
//
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_BITSET_H
#define EASTL_BITSET_H


#include <EASTL/internal/config.h>
#include <EASTL/algorithm.h>

#ifdef _MSC_VER
    #pragma warning(push, 0)
#endif
#include <stddef.h>
#ifdef __MWERKS__
    #include <../Include/string.h> // Force the compiler to use the std lib header.
#else
    #include <string.h>
#endif
#ifdef _MSC_VER
    #pragma warning(pop)
#endif

#if EASTL_EXCEPTIONS_ENABLED
    #ifdef _MSC_VER
        #pragma warning(push, 0)
    #endif
    #include <stdexcept> // std::out_of_range, std::length_error.
    #ifdef _MSC_VER
        #pragma warning(pop)
    #endif
#endif

#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable: 4127)  // Conditional expression is constant
#elif defined(__SNC__)
    #pragma control %push diag
    #pragma diag_suppress=187       // Pointless comparison of unsigned integer with zero
#endif


namespace eastl
{

    /// BitsetWordType
    ///
    /// Defines the integral data type used by bitset.
    /// The C++ standard specifies that the std::bitset word type be unsigned long, 
    /// but that isn't necessarily the most efficient data type for the given platform.
    /// We can follow the standard and be potentially less efficient or we can do what
    /// is more efficient but less like the C++ std::bitset.
    ///
    #if(EA_PLATFORM_WORD_SIZE == 4)
        typedef uint32_t BitsetWordType;
        const   uint32_t kBitsPerWord      = 32;
        const   uint32_t kBitsPerWordMask  = 31;
        const   uint32_t kBitsPerWordShift =  5;
    #else
        typedef uint64_t BitsetWordType;
        const   uint32_t kBitsPerWord      = 64;
        const   uint32_t kBitsPerWordMask  = 63;
        const   uint32_t kBitsPerWordShift =  6;
    #endif



    /// BITSET_WORD_COUNT
    ///
    /// Defines the number of words we use, based on the number of bits.
    ///
    #if !defined(__GNUC__) || (__GNUC__ >= 3) // GCC 2.x can't handle the simpler declaration below.
        #define BITSET_WORD_COUNT(nBitCount) (N == 0 ? 1 : ((N - 1) / (8 * sizeof(BitsetWordType)) + 1))
    #else
        #define BITSET_WORD_COUNT(nBitCount) ((N - 1) / (8 * sizeof(BitsetWordType)) + 1)
    #endif



    /// BitsetBase
    ///
    /// This is a default implementation that works for any number of words.
    ///
    template <size_t NW> // Templated on the number of words used to hold the bitset.
    struct BitsetBase
    {
        typedef BitsetWordType word_type;
        typedef BitsetBase<NW> this_type;
      #if EASTL_BITSET_SIZE_T
        typedef size_t         size_type;
      #else
        typedef eastl_size_t   size_type;
      #endif

    public:
        word_type mWord[NW];

    public:
        BitsetBase();
        BitsetBase(uint32_t value);

        void operator&=(const this_type& x);
        void operator|=(const this_type& x);
        void operator^=(const this_type& x);

        void operator<<=(size_type n);
        void operator>>=(size_type n);

        void flip();
        void set();
        void set(size_type i, bool value);
        void reset();

        bool operator==(const this_type& x) const;

        bool      any() const;
        size_type count() const;

        unsigned long to_ulong() const;

        word_type& DoGetWord(size_type i);
        word_type  DoGetWord(size_type i) const;

        size_type DoFindFirst() const;
        size_type DoFindNext(size_type last_find) const;

        size_type DoFindLast() const;
        size_type DoFindPrev(size_type last_find) const;

    }; // class BitsetBase



    /// BitsetBase<1>
    /// 
    /// This is a specialization for a bitset that fits within one word.
    ///
    template <>
    struct BitsetBase<1>
    {
        typedef BitsetWordType word_type;
        typedef BitsetBase<1>  this_type;
      #if EASTL_BITSET_SIZE_T
        typedef size_t         size_type;
      #else
        typedef eastl_size_t   size_type;
      #endif

    public:
        word_type mWord[1]; // Defined as an array of 1 so that bitset can treat this BitsetBase like others.

    public:
        BitsetBase();
        BitsetBase(uint32_t value);

        void operator&=(const this_type& x);
        void operator|=(const this_type& x);
        void operator^=(const this_type& x);

        void operator<<=(size_type n);
        void operator>>=(size_type n);

        void flip();
        void set();
        void set(size_type i, bool value);
        void reset();

        bool operator==(const this_type& x) const;

        bool      any() const;
        size_type count() const;

        unsigned long to_ulong() const;

        word_type& DoGetWord(size_type);
        word_type  DoGetWord(size_type) const;

        size_type DoFindFirst() const;
        size_type DoFindNext(size_type last_find) const;

        size_type DoFindLast() const;
        size_type DoFindPrev(size_type last_find) const;

    }; // BitsetBase<1>



    /// BitsetBase<2>
    /// 
    /// This is a specialization for a bitset that fits within two words.
    /// The difference here is that we avoid branching (ifs and loops).
    ///
    template <>
    struct BitsetBase<2>
    {
        typedef BitsetWordType word_type;
        typedef BitsetBase<2>  this_type;
      #if EASTL_BITSET_SIZE_T
        typedef size_t         size_type;
      #else
        typedef eastl_size_t   size_type;
      #endif

    public:
        word_type mWord[2];

    public:
        BitsetBase();
        BitsetBase(uint32_t value);

        void operator&=(const this_type& x);
        void operator|=(const this_type& x);
        void operator^=(const this_type& x);

        void operator<<=(size_type n);
        void operator>>=(size_type n);

        void flip();
        void set();
        void set(size_type i, bool value);
        void reset();

        bool operator==(const this_type& x) const;

        bool      any() const;
        size_type count() const;

        unsigned long to_ulong() const;

        word_type& DoGetWord(size_type);
        word_type  DoGetWord(size_type) const;

        size_type DoFindFirst() const;
        size_type DoFindNext(size_type last_find) const;

        size_type DoFindLast() const;
        size_type DoFindPrev(size_type last_find) const;

    }; // BitsetBase<2>




    /// bitset
    ///
    /// Implements a bitset much like the C++ std::bitset.
    ///
    /// As of this writing we don't have an implementation of bitset<0>,
    /// as it is deemed an academic exercise that nobody should actually
    /// use and it would increase code space.
    ///
    /// Note: bitset shifts of a magnitude >= sizeof(BitsetWordType)
    /// (e.g. shift of 32 on a 32 bit system) are not guaranteed to work
    /// properly. This is because some systems (e.g. Intel x86) take the
    /// shift value and mod it to the word size and thus a shift of 32
    /// can become a shift of 0 on a 32 bit system. We don't attempt to
    /// resolve this behaviour in this class because doing so would lead
    /// to a less efficient implementation and the vast majority of the 
    /// time the user doesn't do shifts of >= word size. You can work 
    /// around this by implementing a shift of 32 as two shifts of 16.
    ///
    template <size_t N>
    class bitset : private BitsetBase<BITSET_WORD_COUNT(N)>
    {
    public:
        typedef BitsetBase<BITSET_WORD_COUNT(N)>    base_type;
        typedef bitset<N>                           this_type;
        typedef BitsetWordType                      word_type;
        typedef typename base_type::size_type       size_type;

        enum
        {
            kSize      = N,
            kWordCount = BITSET_WORD_COUNT(N),
            kNW        = kWordCount             // This name is deprecated.
        };

        using base_type::mWord;
        using base_type::DoGetWord;
        using base_type::DoFindFirst;
        using base_type::DoFindNext;
        using base_type::DoFindLast;
        using base_type::DoFindPrev;

    public:
        /// reference
        ///
        /// A reference is a reference to a specific bit in the bitset.
        /// The C++ standard specifies that this be a nested class, 
        /// though it is not clear if a non-nested reference implementation
        /// would be non-conforming.
        ///
        class reference
        {
        protected:
            friend class bitset;

            word_type* mpBitWord;
            size_type  mnBitIndex;
        
            reference(){} // The C++ standard specifies that this is private.
    
        public:
            reference(const bitset& x, size_type i);

            reference& operator=(bool value);
            reference& operator=(const reference& x);

            bool operator~() const;
            operator bool() const // Defined inline because CodeWarrior fails to be able to compile it outside.
               { return (*mpBitWord & (static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask))) != 0; }

            reference& flip();
        };

    public:
        friend class reference;

        bitset();
        bitset(uint32_t value);

        // We don't define copy constructor and operator= because 
        // the compiler-generated versions will suffice.

        this_type& operator&=(const this_type& x);
        this_type& operator|=(const this_type& x);
        this_type& operator^=(const this_type& x);

        this_type& operator<<=(size_type n);
        this_type& operator>>=(size_type n);

        this_type& set();
        this_type& set(size_type i, bool value = true);

        this_type& reset();
        this_type& reset(size_type i);
            
        this_type& flip();
        this_type& flip(size_type i);
        this_type  operator~() const;

        reference operator[](size_type i);
        bool      operator[](size_type i) const;

        const word_type* data() const;
        word_type*       data();

        unsigned long to_ulong() const;

        size_type count() const;
        size_type size() const;

        bool operator==(const this_type& x) const;
        bool operator!=(const this_type& x) const;

        bool test(size_type i) const;
        bool any() const;
        bool none() const;

        this_type operator<<(size_type n) const;
        this_type operator>>(size_type n) const;

        // Finds the index of the first "on" bit, returns kSize if none are set.
        size_type find_first() const;

        // Finds the index of the next "on" bit after last_find, returns kSize if none are set.
        size_type find_next(size_type last_find) const;

        // Finds the index of the last "on" bit, returns kSize if none are set.
        size_type find_last() const;

        // Finds the index of the last "on" bit before last_find, returns kSize if none are set.
        size_type find_prev(size_type last_find) const;

    }; // bitset







    /// BitsetCountBits
    ///
    /// This is a fast trick way to count bits without branches nor memory accesses.
    ///
    #if(EA_PLATFORM_WORD_SIZE == 4)
        inline uint32_t BitsetCountBits(uint32_t x)
        {
            x = x - ((x >> 1) & 0x55555555);
            x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
            x = (x + (x >> 4)) & 0x0F0F0F0F;
            return (uint32_t)((x * 0x01010101) >> 24);
        }
    #else
        inline uint32_t BitsetCountBits(uint64_t x)
        {
            // GCC 3.x's implementation of UINT64_C is broken and fails to deal with 
            // the code below correctly. So we make a workaround for it. Earlier and 
            // later versions of GCC don't have this bug.
            #if defined(__GNUC__) && (__GNUC__ == 3)
                x = x - ((x >> 1) & 0x5555555555555555ULL);
                x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
                x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
                return (uint32_t)((x * 0x0101010101010101ULL) >> 56);
            #else
                x = x - ((x >> 1) & UINT64_C(0x5555555555555555));
                x = (x & UINT64_C(0x3333333333333333)) + ((x >> 2) & UINT64_C(0x3333333333333333));
                x = (x + (x >> 4)) & UINT64_C(0x0F0F0F0F0F0F0F0F);
                return (uint32_t)((x * UINT64_C(0x0101010101010101)) >> 56);
            #endif
        }
    #endif

    // const static char kBitsPerUint16[16] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
    #define EASTL_BITSET_COUNT_STRING "\0\1\1\2\1\2\2\3\1\2\2\3\2\3\3\4"



    ///////////////////////////////////////////////////////////////////////////
    // BitsetBase
    //
    // We tried two forms of array access here:
    //     for(word_type *pWord(mWord), *pWordEnd(mWord + NW); pWord < pWordEnd; ++pWord)
    //         *pWord = ...
    // and
    //     for(size_t i = 0; i < NW; i++)
    //         mWord[i] = ...
    //
    // For our tests (~NW < 16), the latter (using []) access resulted in faster code. 
    ///////////////////////////////////////////////////////////////////////////

    template <size_t NW>
    inline BitsetBase<NW>::BitsetBase()
    {
        reset();
    }


    template <size_t NW>
    inline BitsetBase<NW>::BitsetBase(uint32_t value)
    {
        // This implementation assumes that sizeof(value) <= sizeof(BitsetWordType).
        EASTL_CT_ASSERT(sizeof(value) <= sizeof(BitsetWordType));

        reset();
        mWord[0] = static_cast<word_type>(value);
    }


    template <size_t NW>
    inline void BitsetBase<NW>::operator&=(const this_type& x)
    {
        for(size_t i = 0; i < NW; i++)
            mWord[i] &= x.mWord[i];
    }


    template <size_t NW>
    inline void BitsetBase<NW>::operator|=(const this_type& x)
    {
        for(size_t i = 0; i < NW; i++)
            mWord[i] |= x.mWord[i];
    }


    template <size_t NW>
    inline void BitsetBase<NW>::operator^=(const this_type& x)
    {
        for(size_t i = 0; i < NW; i++)
            mWord[i] ^= x.mWord[i];
    }


    template <size_t NW>
    inline void BitsetBase<NW>::operator<<=(size_type n)
    {
        const size_type nWordShift = (size_type)(n >> kBitsPerWordShift);

        if(nWordShift)
        {
            for(int i = (int)(NW - 1); i >= 0; --i)
                mWord[i] = (nWordShift <= (size_type)i) ? mWord[i - nWordShift] : (word_type)0;
        }

        if(n &= kBitsPerWordMask)
        {
            for(size_t i = (NW - 1); i > 0; --i)
                mWord[i] = (word_type)((mWord[i] << n) | (mWord[i - 1] >> (kBitsPerWord - n)));
            mWord[0] <<= n;
        }

        // We let the parent class turn off any upper bits.
    }


    template <size_t NW>
    inline void BitsetBase<NW>::operator>>=(size_type n)
    {
        const size_type nWordShift = (size_type)(n >> kBitsPerWordShift);

        if(nWordShift)
        {
            for(size_t i = 0; i < NW; ++i)
                mWord[i] = ((nWordShift < (NW - i)) ? mWord[i + nWordShift] : (word_type)0);
        }

        if(n &= kBitsPerWordMask)
        {
            for(size_t i = 0; i < (NW - 1); ++i)
                mWord[i] = (word_type)((mWord[i] >> n) | (mWord[i + 1] << (kBitsPerWord - n)));
            mWord[NW - 1] >>= n;
        }
    }


    template <size_t NW>
    inline void BitsetBase<NW>::flip()
    {
        for(size_t i = 0; i < NW; i++)
            mWord[i] = ~mWord[i];
        // We let the parent class turn off any upper bits.
    }


    template <size_t NW>
    inline void BitsetBase<NW>::set()
    {
        for(size_t i = 0; i < NW; i++)
            mWord[i] = ~static_cast<word_type>(0);
        // We let the parent class turn off any upper bits.
    }


    template <size_t NW>
    inline void BitsetBase<NW>::set(size_type i, bool value)
    {
        if(value)
            mWord[i >> kBitsPerWordShift] |=  (static_cast<word_type>(1) << (i & kBitsPerWordMask));
        else
            mWord[i >> kBitsPerWordShift] &= ~(static_cast<word_type>(1) << (i & kBitsPerWordMask));
    }


    template <size_t NW>
    inline void BitsetBase<NW>::reset()
    {
        if(NW > 16) // This is a constant expression and should be optimized away.
        {
            // This will be fastest if compiler intrinsic function optimizations are enabled.
            memset(mWord, 0, sizeof(mWord));
        }
        else
        {
            for(size_t i = 0; i < NW; i++)
                mWord[i] = 0;
        }
    }


    template <size_t NW>
    inline bool BitsetBase<NW>::operator==(const this_type& x) const
    {
        for(size_t i = 0; i < NW; i++)
        {
            if(mWord[i] != x.mWord[i])
                return false;
        }
        return true;
    }


    template <size_t NW>
    inline bool BitsetBase<NW>::any() const
    {
        for(size_t i = 0; i < NW; i++)
        {
            if(mWord[i])
                return true;
        }
        return false;
    }


    template <size_t NW>
    inline typename BitsetBase<NW>::size_type
    BitsetBase<NW>::count() const
    {
        size_type n = 0;

        for(size_t i = 0; i < NW; i++)
        {
            #if defined(__GNUC__) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 304) && !defined(__SNC__) && !defined(EA_PLATFORM_ANDROID) // GCC 3.4 or later
                #if(EA_PLATFORM_WORD_SIZE == 4)
                    n += (size_type)__builtin_popcountl(mWord[i]);
                #else
                    n += (size_type)__builtin_popcountll(mWord[i]);
                #endif
            #elif defined(__GNUC__) && (__GNUC__ < 3)
                n +=  BitsetCountBits(mWord[i]); // GCC 2.x compiler inexplicably blows up on the code below.
            #else
                for(word_type w = mWord[i]; w; w >>= 4)
                    n += EASTL_BITSET_COUNT_STRING[w & 0xF];

                // Version which seems to run slower in benchmarks:
                // n +=  BitsetCountBits(mWord[i]);
            #endif

        }
        return n;
    }


    template <size_t NW>
    inline unsigned long BitsetBase<NW>::to_ulong() const
    {
        #if EASTL_EXCEPTIONS_ENABLED
            for(size_t i = 1; i < NW; ++i)
            {
                if(mWord[i])
                    throw overflow_error("BitsetBase::to_ulong");
            }
        #endif
        return (unsigned long)mWord[0]; // Todo: We need to deal with the case whereby sizeof(word_type) < sizeof(unsigned long)
    }


    template <size_t NW>
    inline typename BitsetBase<NW>::word_type&
    BitsetBase<NW>::DoGetWord(size_type i)
    {
        return mWord[i >> kBitsPerWordShift];
    }


    template <size_t NW>
    inline typename BitsetBase<NW>::word_type
    BitsetBase<NW>::DoGetWord(size_type i) const
    {
        return mWord[i >> kBitsPerWordShift];
    }


    #if(EA_PLATFORM_WORD_SIZE == 4)
        inline uint32_t GetFirstBit(uint32_t x)
        {
            if(x)
            {
                uint32_t n = 1;

                if((x & 0x0000FFFF) == 0) { n += 16; x >>= 16; }
                if((x & 0x000000FF) == 0) { n +=  8; x >>=  8; }
                if((x & 0x0000000F) == 0) { n +=  4; x >>=  4; }
                if((x & 0x00000003) == 0) { n +=  2; x >>=  2; }

                return (n - ((uint32_t)x & 1));
            }

            return 32;
        }
    #else
        inline uint32_t GetFirstBit(uint64_t x)
        {
            if(x)
            {
                uint32_t n = 1;

                if((x & 0xFFFFFFFF) == 0) { n += 32; x >>= 32; }
                if((x & 0x0000FFFF) == 0) { n += 16; x >>= 16; }
                if((x & 0x000000FF) == 0) { n +=  8; x >>=  8; }
                if((x & 0x0000000F) == 0) { n +=  4; x >>=  4; }
                if((x & 0x00000003) == 0) { n +=  2; x >>=  2; }

                return (n - ((uint32_t)x & 1));
            }

            return 64;
        }
    #endif


    template <size_t NW>
    inline typename BitsetBase<NW>::size_type 
    BitsetBase<NW>::DoFindFirst() const
    {
        for(size_type word_index = 0; word_index < NW; ++word_index)
        {
            const size_type fbiw = GetFirstBit(mWord[word_index]);

            if(fbiw != kBitsPerWord)
                return (word_index * kBitsPerWord) + fbiw;
        }

        return (size_type)NW * kBitsPerWord;
    }


    template <size_t NW>
    inline typename BitsetBase<NW>::size_type 
    BitsetBase<NW>::DoFindNext(size_type last_find) const
    {
        // Start looking from the next bit.
        ++last_find;

        // Set initial state based on last find.
        size_type word_index = static_cast<size_type>(last_find >> kBitsPerWordShift);
        size_type bit_index  = static_cast<size_type>(last_find  & kBitsPerWordMask);

        // To do: There probably is a more elegant way to write looping below.
        if(word_index < NW)
        {
            // Mask off previous bits of the word so our search becomes a "find first".
            word_type this_word = mWord[word_index] & (~static_cast<word_type>(0) << bit_index);

            for(;;)
            {
                const size_type fbiw = GetFirstBit(this_word);

                if(fbiw != kBitsPerWord)
                    return (word_index * kBitsPerWord) + fbiw;

                if(++word_index < NW)
                    this_word = mWord[word_index];
                else
                    break;
            }
        }

        return (size_type)NW * kBitsPerWord;
    }


    #if(EA_PLATFORM_WORD_SIZE == 4)
        inline uint32_t GetLastBit(uint32_t x)
        {
            if(x)
            {
                uint32_t n = 0;

                if(x & 0xFFFF0000) { n += 16; x >>= 16; }
                if(x & 0xFFFFFF00) { n +=  8; x >>=  8; }
                if(x & 0xFFFFFFF0) { n +=  4; x >>=  4; }
                if(x & 0xFFFFFFFC) { n +=  2; x >>=  2; }
                if(x & 0xFFFFFFFE) { n +=  1;           }

                return n;
            }

            return 32;
        }
    #else
        inline uint32_t GetLastBit(uint64_t x)
        {
            if(x)
            {
                uint32_t n = 0;

                if(x & UINT64_C(0xFFFFFFFF00000000)) { n += 32; x >>= 32; }
                if(x & 0xFFFF0000)                   { n += 16; x >>= 16; }
                if(x & 0xFFFFFF00)                   { n +=  8; x >>=  8; }
                if(x & 0xFFFFFFF0)                   { n +=  4; x >>=  4; }
                if(x & 0xFFFFFFFC)                   { n +=  2; x >>=  2; }
                if(x & 0xFFFFFFFE)                   { n +=  1;           }

                return n;
            }

            return 64;
        }
    #endif

    template <size_t NW>
    inline typename BitsetBase<NW>::size_type 
    BitsetBase<NW>::DoFindLast() const
    {
        for(size_t word_index = (size_type)NW - 1; word_index < NW; --word_index)
        {
            const size_type lbiw = GetLastBit(mWord[word_index]);

            if(lbiw != kBitsPerWord)
                return (word_index * kBitsPerWord) + lbiw;
        }

        return (size_type)NW * kBitsPerWord;
    }


    template <size_t NW>
    inline typename BitsetBase<NW>::size_type 
    BitsetBase<NW>::DoFindPrev(size_type last_find) const
    {
        if(last_find > 0)
        {
            // Set initial state based on last find.
            size_type word_index = static_cast<size_type>(last_find >> kBitsPerWordShift);
            size_type bit_index  = static_cast<size_type>(last_find  & kBitsPerWordMask);

            // Mask off subsequent bits of the word so our search becomes a "find last".
            word_type mask      = (~static_cast<word_type>(0) >> (kBitsPerWord - 1 - bit_index)) >> 1; // We do two shifts here because many CPUs ignore requests to shift 32 bit integers by 32 bits, which could be the case above.
            word_type this_word = mWord[word_index] & mask;

            for(;;)
            {
                const size_type lbiw = GetLastBit(this_word);

                if(lbiw != kBitsPerWord)
                    return (word_index * kBitsPerWord) + lbiw;

                if(word_index > 0)
                    this_word = mWord[--word_index];
                else
                    break;
            }
        }

        return (size_type)NW * kBitsPerWord;
    }



    ///////////////////////////////////////////////////////////////////////////
    // BitsetBase<1>
    ///////////////////////////////////////////////////////////////////////////

    inline BitsetBase<1>::BitsetBase()
    {
        mWord[0] = 0;
    }


    inline BitsetBase<1>::BitsetBase(uint32_t value)
    {
        // This implementation assumes that sizeof(value) <= sizeof(BitsetWordType).
        EASTL_CT_ASSERT(sizeof(value) <= sizeof(BitsetWordType));

        mWord[0] = static_cast<word_type>(value);
    }


    inline void BitsetBase<1>::operator&=(const this_type& x)
    {
        mWord[0] &= x.mWord[0];
    }


    inline void BitsetBase<1>::operator|=(const this_type& x)
    {
        mWord[0] |= x.mWord[0];
    }


    inline void BitsetBase<1>::operator^=(const this_type& x)
    {
        mWord[0] ^= x.mWord[0];
    }


    inline void BitsetBase<1>::operator<<=(size_type n)
    {
        mWord[0] <<= n;
        // We let the parent class turn off any upper bits.
    }


    inline void BitsetBase<1>::operator>>=(size_type n)
    {
        mWord[0] >>= n;
    }


    inline void BitsetBase<1>::flip()
    {
        mWord[0] = ~mWord[0];
        // We let the parent class turn off any upper bits.
    }


    inline void BitsetBase<1>::set()
    {
        mWord[0] = ~static_cast<word_type>(0);
        // We let the parent class turn off any upper bits.
    }


    inline void BitsetBase<1>::set(size_type i, bool value)
    {
        if(value)
            mWord[0] |=  (static_cast<word_type>(1) << i);
        else
            mWord[0] &= ~(static_cast<word_type>(1) << i);
    }


    inline void BitsetBase<1>::reset()
    {
        mWord[0] = 0;
    }


    inline bool BitsetBase<1>::operator==(const this_type& x) const
    {
        return mWord[0] == x.mWord[0];
    }


    inline bool BitsetBase<1>::any() const
    {
        return mWord[0] != 0;
    }


    inline BitsetBase<1>::size_type
    BitsetBase<1>::count() const
    {
        #if defined(__GNUC__) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 304) && !defined(__SNC__) && !defined(EA_PLATFORM_ANDROID) // GCC 3.4 or later
            #if(EA_PLATFORM_WORD_SIZE == 4)
                return (size_type)__builtin_popcountl(mWord[0]);
            #else
                return (size_type)__builtin_popcountll(mWord[0]);
            #endif
        #elif defined(__GNUC__) && (__GNUC__ < 3)
            return BitsetCountBits(mWord[0]); // GCC 2.x compiler inexplicably blows up on the code below.
        #else
            size_type n = 0;
            for(word_type w = mWord[0]; w; w >>= 4)
                n += EASTL_BITSET_COUNT_STRING[w & 0xF];
            return n;
        #endif
    }


    inline unsigned long BitsetBase<1>::to_ulong() const
    {
        return static_cast<unsigned long>(mWord[0]);
    }


    inline BitsetBase<1>::word_type&
    BitsetBase<1>::DoGetWord(size_type)
    {
        return mWord[0];
    }


    inline BitsetBase<1>::word_type
    BitsetBase<1>::DoGetWord(size_type) const
    {
        return mWord[0];
    }


    inline BitsetBase<1>::size_type
    BitsetBase<1>::DoFindFirst() const
    {
        return GetFirstBit(mWord[0]);
    }


    inline BitsetBase<1>::size_type 
    BitsetBase<1>::DoFindNext(size_type last_find) const
    {
        if(++last_find < kBitsPerWord)
        {
            // Mask off previous bits of word so our search becomes a "find first".
            const word_type this_word = mWord[0] & ((~static_cast<word_type>(0)) << last_find);

            return GetFirstBit(this_word);
        }

        return kBitsPerWord;
    }


    inline BitsetBase<1>::size_type 
    BitsetBase<1>::DoFindLast() const
    {
        return GetLastBit(mWord[0]);
    }


    inline BitsetBase<1>::size_type 
    BitsetBase<1>::DoFindPrev(size_type last_find) const
    {
        if(last_find > 0)
        {
            // Mask off previous bits of word so our search becomes a "find first".
            const word_type this_word = mWord[0] & ((~static_cast<word_type>(0)) >> (kBitsPerWord - last_find));

            return GetLastBit(this_word);
        }

        return kBitsPerWord;
    }




    ///////////////////////////////////////////////////////////////////////////
    // BitsetBase<2>
    ///////////////////////////////////////////////////////////////////////////

    inline BitsetBase<2>::BitsetBase()
    {
        mWord[0] = 0;
        mWord[1] = 0;
    }


    inline BitsetBase<2>::BitsetBase(uint32_t value)
    {
        // This implementation assumes that sizeof(value) <= sizeof(BitsetWordType).
        EASTL_CT_ASSERT(sizeof(value) <= sizeof(BitsetWordType));

        mWord[0] = static_cast<word_type>(value);
        mWord[1] = 0;
    }


    inline void BitsetBase<2>::operator&=(const this_type& x)
    {
        mWord[0] &= x.mWord[0];
        mWord[1] &= x.mWord[1];
    }


    inline void BitsetBase<2>::operator|=(const this_type& x)
    {
        mWord[0] |= x.mWord[0];
        mWord[1] |= x.mWord[1];
    }


    inline void BitsetBase<2>::operator^=(const this_type& x)
    {
        mWord[0] ^= x.mWord[0];
        mWord[1] ^= x.mWord[1];
    }


    inline void BitsetBase<2>::operator<<=(size_type n)
    {
        if(EASTL_UNLIKELY(n >= kBitsPerWord))   // parent expected to handle high bits and n >= 64
        {
            mWord[1] = mWord[0];
            mWord[0] = 0;
            n -= kBitsPerWord;
        }

        mWord[1] = (mWord[1] << n) | (mWord[0] >> (kBitsPerWord - n)); // Intentionally use | instead of +.
        mWord[0] <<= n;
        // We let the parent class turn off any upper bits.
    }


    inline void BitsetBase<2>::operator>>=(size_type n)
    {
        if(EASTL_UNLIKELY(n >= kBitsPerWord))   // parent expected to handle n >= 64
        {
            mWord[0] = mWord[1];
            mWord[1] = 0;
            n -= kBitsPerWord;
        }

        mWord[0] = (mWord[0] >> n) | (mWord[1] << (kBitsPerWord - n)); // Intentionally use | instead of +.
        mWord[1] >>= n;
    }


    inline void BitsetBase<2>::flip()
    {
        mWord[0] = ~mWord[0];
        mWord[1] = ~mWord[1];
        // We let the parent class turn off any upper bits.
    }


    inline void BitsetBase<2>::set()
    {
        mWord[0] = ~static_cast<word_type>(0);
        mWord[1] = ~static_cast<word_type>(0);
        // We let the parent class turn off any upper bits.
    }


    inline void BitsetBase<2>::set(size_type i, bool value)
    {
        if(value)
            mWord[i >> kBitsPerWordShift] |=  (static_cast<word_type>(1) << (i & kBitsPerWordMask));
        else
            mWord[i >> kBitsPerWordShift] &= ~(static_cast<word_type>(1) << (i & kBitsPerWordMask));
    }


    inline void BitsetBase<2>::reset()
    {
        mWord[0] = 0;
        mWord[1] = 0;
    }


    inline bool BitsetBase<2>::operator==(const this_type& x) const
    {
        return (mWord[0] == x.mWord[0]) && (mWord[1] == x.mWord[1]);
    }


    inline bool BitsetBase<2>::any() const
    {
        // Or with two branches: { return (mWord[0] != 0) || (mWord[1] != 0); }
        return (mWord[0] | mWord[1]) != 0; 
    }


    inline BitsetBase<2>::size_type
    BitsetBase<2>::count() const
    {
        #if defined(__GNUC__) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 304) && !defined(__SNC__) && !defined(EA_PLATFORM_ANDROID) // GCC 3.4 or later
            #if(EA_PLATFORM_WORD_SIZE == 4)
                return (size_type)__builtin_popcountl(mWord[0])  + (size_type)__builtin_popcountl(mWord[1]);
            #else
                return (size_type)__builtin_popcountll(mWord[0]) + (size_type)__builtin_popcountll(mWord[1]);
            #endif

        #else
            return BitsetCountBits(mWord[0]) + BitsetCountBits(mWord[1]);
        #endif
    }


    inline unsigned long BitsetBase<2>::to_ulong() const
    {
        #if EASTL_EXCEPTIONS_ENABLED
            if(mWord[1])
                throw overflow_error("BitsetBase::to_ulong");
        #endif
        return (unsigned long)mWord[0]; // Todo: We need to deal with the case whereby sizeof(word_type) < sizeof(unsigned long)
    }


    inline BitsetBase<2>::word_type&
    BitsetBase<2>::DoGetWord(size_type i)
    {
        return mWord[i >> kBitsPerWordShift];
    }


    inline BitsetBase<2>::word_type
    BitsetBase<2>::DoGetWord(size_type i) const
    {
        return mWord[i >> kBitsPerWordShift];
    }


    inline BitsetBase<2>::size_type 
    BitsetBase<2>::DoFindFirst() const
    {
        size_type fbiw = GetFirstBit(mWord[0]);

        if(fbiw != kBitsPerWord)
            return fbiw;

        fbiw = GetFirstBit(mWord[1]);

        if(fbiw != kBitsPerWord)
            return kBitsPerWord + fbiw;

        return 2 * kBitsPerWord;
    }


    inline BitsetBase<2>::size_type 
    BitsetBase<2>::DoFindNext(size_type last_find) const
    {
        // If the last find was in the first word, we must check it and then possibly the second.
        if(++last_find < (size_type)kBitsPerWord)
        {
            // Mask off previous bits of word so our search becomes a "find first".
            word_type this_word = mWord[0] & ((~static_cast<word_type>(0)) << last_find);

            // Step through words.
            size_type fbiw = GetFirstBit(this_word);

            if(fbiw != kBitsPerWord)
                return fbiw;

            fbiw = GetFirstBit(mWord[1]);

            if(fbiw != kBitsPerWord)
                return kBitsPerWord + fbiw;
        }
        else if(last_find < (size_type)(2 * kBitsPerWord))
        {
            // The last find was in the second word, remove the bit count of the first word from the find.
            last_find -= kBitsPerWord;

            // Mask off previous bits of word so our search becomes a "find first".
            word_type this_word = mWord[1] & ((~static_cast<word_type>(0)) << last_find);

            const size_type fbiw = GetFirstBit(this_word);

            if(fbiw != kBitsPerWord)
                return kBitsPerWord + fbiw;
        }

        return 2 * kBitsPerWord;
    }


    inline BitsetBase<2>::size_type 
    BitsetBase<2>::DoFindLast() const
    {
        size_type lbiw = GetLastBit(mWord[1]);

        if(lbiw != kBitsPerWord)
            return kBitsPerWord + lbiw;

        lbiw = GetLastBit(mWord[0]);

        if(lbiw != kBitsPerWord)
            return lbiw;

        return 2 * kBitsPerWord;
    }


    inline BitsetBase<2>::size_type 
    BitsetBase<2>::DoFindPrev(size_type last_find) const
    {
        // If the last find was in the second word, we must check it and then possibly the first.
        if(last_find > (size_type)kBitsPerWord)
        {
            // This has the same effect as last_find %= kBitsPerWord in our case.
            last_find -= kBitsPerWord;

            // Mask off previous bits of word so our search becomes a "find first".
            word_type this_word = mWord[1] & ((~static_cast<word_type>(0)) >> (kBitsPerWord - last_find));

            // Step through words.
            size_type lbiw = GetLastBit(this_word);

            if(lbiw != kBitsPerWord)
                return kBitsPerWord + lbiw;

            lbiw = GetLastBit(mWord[0]);

            if(lbiw != kBitsPerWord)
                return lbiw;
        }
        else if(last_find != 0)
        {
            // Mask off previous bits of word so our search becomes a "find first".
            word_type this_word = mWord[0] & ((~static_cast<word_type>(0)) >> (kBitsPerWord - last_find));

            const size_type lbiw = GetLastBit(this_word);

            if(lbiw != kBitsPerWord)
                return lbiw;
        }

        return 2 * kBitsPerWord;
    }



    ///////////////////////////////////////////////////////////////////////////
    // bitset::reference
    ///////////////////////////////////////////////////////////////////////////

    template <size_t N>
    inline bitset<N>::reference::reference(const bitset& x, size_type i)
        : mpBitWord(&const_cast<bitset&>(x).DoGetWord(i)),
          mnBitIndex(i & kBitsPerWordMask)
    {   // We have an issue here because the above is casting away the const-ness of the source bitset.
        // Empty
    }


    template <size_t N>
    inline typename bitset<N>::reference&
    bitset<N>::reference::operator=(bool value)
    {
        if(value)
            *mpBitWord |=  (static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask));
        else
            *mpBitWord &= ~(static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask));
        return *this;
    }


    template <size_t N>
    inline typename bitset<N>::reference&
    bitset<N>::reference::operator=(const reference& x)
    {
        if(*x.mpBitWord & (static_cast<word_type>(1) << (x.mnBitIndex & kBitsPerWordMask)))
            *mpBitWord |=  (static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask));
        else
            *mpBitWord &= ~(static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask));
        return *this;
    }


    template <size_t N>
    inline bool bitset<N>::reference::operator~() const
    {
        return (*mpBitWord & (static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask))) == 0;
    }


    //Defined inline in the class because Metrowerks fails to be able to compile it here.
    //template <size_t N>
    //inline bitset<N>::reference::operator bool() const
    //{
    //    return (*mpBitWord & (static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask))) != 0;
    //}


    template <size_t N>
    inline typename bitset<N>::reference&
    bitset<N>::reference::flip()
    {
        *mpBitWord ^= static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask);
        return *this;
    }




    ///////////////////////////////////////////////////////////////////////////
    // bitset
    ///////////////////////////////////////////////////////////////////////////

    template <size_t N>
    inline bitset<N>::bitset()
        : base_type()
    {
        // Empty. The base class will set all bits to zero.
    }


    template <size_t N>
    inline bitset<N>::bitset(uint32_t value)
        : base_type(value)
    {
        if((N & kBitsPerWordMask) || (N == 0)) // If there are any high bits to clear... (If we didn't have this check, then the code below would do the wrong thing when N == 32.
            mWord[kNW - 1] &= ~(~static_cast<word_type>(0) << (N & kBitsPerWordMask)); // This clears any high unused bits.
    }


    template <size_t N>
    inline typename bitset<N>::this_type&
    bitset<N>::operator&=(const this_type& x)
    {
        base_type::operator&=(x);
        return *this;
    }


    template <size_t N>
    inline typename bitset<N>::this_type&
    bitset<N>::operator|=(const this_type& x)
    {
        base_type::operator|=(x);
        return *this;
    }


    template <size_t N>
    inline typename bitset<N>::this_type&
    bitset<N>::operator^=(const this_type& x)
    {
        base_type::operator^=(x);
        return *this;
    }


    template <size_t N>
    inline typename bitset<N>::this_type&
    bitset<N>::operator<<=(size_type n)
    {
        if(EASTL_LIKELY((intptr_t)n < (intptr_t)N))
        {
            base_type::operator<<=(n);
            if((N & kBitsPerWordMask) || (N == 0)) // If there are any high bits to clear... (If we didn't have this check, then the code below would do the wrong thing when N == 32.
                mWord[kNW - 1] &= ~(~static_cast<word_type>(0) << (N & kBitsPerWordMask)); // This clears any high unused bits. We need to do this so that shift operations proceed correctly.
        }
        else
            base_type::reset();
        return *this;
    }


    template <size_t N>
    inline typename bitset<N>::this_type&
    bitset<N>::operator>>=(size_type n)
    {
        if(EASTL_LIKELY(n < N))
            base_type::operator>>=(n);
        else
            base_type::reset();
        return *this;
    }


    template <size_t N>
    inline typename bitset<N>::this_type&
    bitset<N>::set()
    {
        base_type::set(); // This sets all bits.
        if((N & kBitsPerWordMask) || (N == 0)) // If there are any high bits to clear... (If we didn't have this check, then the code below would do the wrong thing when N == 32.
            mWord[kNW - 1] &= ~(~static_cast<word_type>(0) << (N & kBitsPerWordMask)); // This clears any high unused bits. We need to do this so that shift operations proceed correctly.
        return *this;
    }


    template <size_t N>
    inline typename bitset<N>::this_type&
    bitset<N>::set(size_type i, bool value)
    {
        if(i < N)
            base_type::set(i, value);

        #if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(!(i < N)))
                EASTL_FAIL_MSG("bitset::set -- out of range");
        #endif

        #if EASTL_EXCEPTIONS_ENABLED
            else
                throw out_of_range("bitset::set");
        #endif
        return *this;
    }


    template <size_t N>
    inline typename bitset<N>::this_type&
    bitset<N>::reset()
    {
        base_type::reset();
        return *this;
    }


    template <size_t N>
    inline typename bitset<N>::this_type&
    bitset<N>::reset(size_type i)
    {
        if(EASTL_LIKELY(i < N))
            DoGetWord(i) &= ~(static_cast<word_type>(1) << (i & kBitsPerWordMask));

        #if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(!(i < N)))
                EASTL_FAIL_MSG("bitset::reset -- out of range");
        #endif

        #if EASTL_EXCEPTIONS_ENABLED
            else
                throw out_of_range("bitset::reset");
        #endif
        return *this;
    }

        
    template <size_t N>
    inline typename bitset<N>::this_type&
    bitset<N>::flip()
    {
        base_type::flip();
        if((N & kBitsPerWordMask) || (N == 0)) // If there are any high bits to clear... (If we didn't have this check, then the code below would do the wrong thing when N == 32.
            mWord[kNW - 1] &= ~(~static_cast<word_type>(0) << (N & kBitsPerWordMask)); // This clears any high unused bits. We need to do this so that shift operations proceed correctly.
        return *this;
    }


    template <size_t N>
    inline typename bitset<N>::this_type&
    bitset<N>::flip(size_type i)
    {
        if(EASTL_LIKELY(i < N))
            DoGetWord(i) ^= (static_cast<word_type>(1) << (i & kBitsPerWordMask));

        #if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(!(i < N)))
                EASTL_FAIL_MSG("bitset::flip -- out of range");
        #endif

        #if EASTL_EXCEPTIONS_ENABLED
            else
                throw out_of_range("bitset::flip");
        #endif
        return *this;
    }
        

    template <size_t N>
    inline typename bitset<N>::this_type
    bitset<N>::operator~() const
    {
        return this_type(*this).flip();
    }


    template <size_t N>
    inline typename bitset<N>::reference
    bitset<N>::operator[](size_type i)
    {
        #if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(!(i < N)))
                EASTL_FAIL_MSG("bitset::operator[] -- out of range");
        #endif

        return reference(*this, i);
    }


    template <size_t N>
    inline bool bitset<N>::operator[](size_type i) const
    {
        #if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(!(i < N)))
                EASTL_FAIL_MSG("bitset::operator[] -- out of range");
        #endif

        return (DoGetWord(i) & (static_cast<word_type>(1) << (i & kBitsPerWordMask))) != 0;
    }


    template <size_t N>
    inline const typename bitset<N>::word_type* bitset<N>::data() const
    {
        return base_type::mWord;
    }


    template <size_t N>
    inline typename bitset<N>::word_type* bitset<N>::data()
    {
        return base_type::mWord;
    }


    template <size_t N>
    inline unsigned long bitset<N>::to_ulong() const
    {
        return base_type::to_ulong();
    }


    template <size_t N>
    inline typename bitset<N>::size_type
    bitset<N>::count() const
    {
        return base_type::count();
    }


    template <size_t N>
    inline typename bitset<N>::size_type
    bitset<N>::size() const
    {
        return (size_type)N;
    }


    template <size_t N>
    inline bool bitset<N>::operator==(const this_type& x) const
    {
        return base_type::operator==(x);
    }


    template <size_t N>
    inline bool bitset<N>::operator!=(const this_type& x) const
    {
        return !base_type::operator==(x);
    }


    template <size_t N>
    inline bool bitset<N>::test(size_type i) const
    {
        if(EASTL_LIKELY(i < N))
            return (DoGetWord(i) & (static_cast<word_type>(1) << (i & kBitsPerWordMask))) != 0;

        #if EASTL_ASSERT_ENABLED
            if(EASTL_UNLIKELY(!(i < N)))
                EASTL_FAIL_MSG("bitset::test -- out of range");
        #endif

        #if EASTL_EXCEPTIONS_ENABLED
            else
                throw out_of_range("bitset::test");
        #endif
        return false;
    }


    template <size_t N>
    inline bool bitset<N>::any() const
    {
        return base_type::any();
    }


    template <size_t N>
    inline bool bitset<N>::none() const
    {
        return !base_type::any();
    }


    template <size_t N>
    inline typename bitset<N>::this_type
    bitset<N>::operator<<(size_type n) const
    {
        return this_type(*this).operator<<=(n);
    }


    template <size_t N>
    inline typename bitset<N>::this_type
    bitset<N>::operator>>(size_type n) const
    {
        return this_type(*this).operator>>=(n);
    }


    template <size_t N>
    inline typename bitset<N>::size_type
    bitset<N>::find_first() const
    {
        const size_type i = base_type::DoFindFirst();

        if(i < (kNW * kBitsPerWord)) // This multiplication is a compile-time constant.
            return i;

        return kSize;
    }


    template <size_t N>
    inline typename bitset<N>::size_type
    bitset<N>::find_next(size_type last_find) const
    {
        const size_type i = base_type::DoFindNext(last_find);

        if(i < (kNW * kBitsPerWord))// This multiplication is a compile-time constant.
            return i;

        return kSize;
    }


    template <size_t N>
    inline typename bitset<N>::size_type
    bitset<N>::find_last() const
    {
        const size_type i = base_type::DoFindLast();

        if(i < (kNW * kBitsPerWord)) // This multiplication is a compile-time constant.
            return i;

        return kSize;
    }


    template <size_t N>
    inline typename bitset<N>::size_type
    bitset<N>::find_prev(size_type last_find) const
    {
        const size_type i = base_type::DoFindPrev(last_find);

        if(i < (kNW * kBitsPerWord))// This multiplication is a compile-time constant.
            return i;

        return kSize;
    }



    ///////////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////////

    template <size_t N>
    inline bitset<N> operator&(const bitset<N>& a, const bitset<N>& b)
    {
        // We get betting inlining when we don't declare temporary variables.
        return bitset<N>(a).operator&=(b);
    }


    template <size_t N>
    inline bitset<N> operator|(const bitset<N>& a, const bitset<N>& b)
    {
        return bitset<N>(a).operator|=(b);
    }


    template <size_t N>
    inline bitset<N> operator^(const bitset<N>& a, const bitset<N>& b)
    {
        return bitset<N>(a).operator^=(b);
    }


} // namespace eastl


#if defined(_MSC_VER)
    #pragma warning(pop)
#elif defined(__SNC__)
    #pragma control %pop diag
#endif


#endif // Header include guard












