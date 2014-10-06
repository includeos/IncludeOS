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
// EASTL/allocator.h
//
// Copyright (c) 2005, Electronic Arts. All rights reserved.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_ALLOCATOR_H
#define EASTL_ALLOCATOR_H


#include <EASTL/internal/config.h>
#include <stddef.h>


#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4189) // local variable is initialized but not referenced
#endif


namespace eastl
{

    /// EASTL_ALLOCATOR_DEFAULT_NAME
    ///
    /// Defines a default allocator name in the absence of a user-provided name.
    ///
#ifndef EASTL_ALLOCATOR_DEFAULT_NAME
#  define EASTL_ALLOCATOR_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX // Unless the user overrides something, this is "EASTL".
#endif


    /// alloc_flags
    ///
    /// Defines allocation flags.
    ///
    enum alloc_flags 
    {
        MEM_TEMP = 0, // Low memory, not necessarily actually temporary.
        MEM_PERM = 1  // High memory, for things that won't be unloaded.
    };


    /// allocator
    ///
    /// In this allocator class, note that it is not templated on any type and
    /// instead it simply allocates blocks of memory much like the C malloc and
    /// free functions. It can be thought of as similar to C++ std::allocator<char>.
    /// The flags parameter has meaning that is specific to the allocation 
    ///
    class EASTL_API allocator
    {
    public:
        typedef eastl_size_t size_type;

        EASTL_ALLOCATOR_EXPLICIT allocator(const char* pName = EASTL_NAME_VAL(EASTL_ALLOCATOR_DEFAULT_NAME));
        allocator(const allocator& x);
        allocator(const allocator& x, const char* pName);

        allocator& operator=(const allocator& x);

        void* allocate(size_t n, int flags = 0);
        void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0);
        void  deallocate(void* p, size_t n);

        const char* get_name() const;
        void        set_name(const char* pName);

    protected:
#if EASTL_NAME_ENABLED
            const char* mpName; // Debug name, used to track memory.
#endif
    };

    bool operator==(const allocator& a, const allocator& b);
    bool operator!=(const allocator& a, const allocator& b);

    EASTL_API allocator* GetDefaultAllocator();
    EASTL_API allocator* SetDefaultAllocator(allocator* pAllocator);



    /// get_default_allocator
    ///
    /// This templated function allows the user to implement a default allocator
    /// retrieval function that any part of EASTL can use. EASTL containers take
    /// an Allocator parameter which identifies an Allocator class to use. But 
    /// different kinds of allocators have different mechanisms for retrieving 
    /// a default allocator instance, and some don't even intrinsically support
    /// such functionality. The user can override this get_default_allocator 
    /// function in order to provide the glue between EASTL and whatever their
    /// system's default allocator happens to be.
    ///
    /// Example usage:
    ///     MyAllocatorType* gpSystemAllocator;
    ///     
    ///     MyAllocatorType* get_default_allocator(const MyAllocatorType*)
    ///         { return gpSystemAllocator; }
    ///
    template <typename Allocator>
    inline Allocator* get_default_allocator(const Allocator*)
    {
        return NULL; // By default we return NULL; the user must make specialization of this function in order to provide their own implementation.
    }

    inline EASTLAllocatorType* get_default_allocator(const EASTLAllocatorType*)
    {
        return EASTLAllocatorDefault(); // For the built-in allocator EASTLAllocatorType, we happen to already have a function for returning the default allocator instance, so we provide it.
    }


    /// default_allocfreemethod
    ///
    /// Implements a default allocfreemethod which uses the default global allocator.
    /// This version supports only default alignment.
    ///
    inline void* default_allocfreemethod(size_t n, void* pBuffer, void* /*pContext*/)
    {
        EASTLAllocatorType* const pAllocator = EASTLAllocatorDefault();

        if(pBuffer) // If freeing...
        {
            EASTLFree(*pAllocator, pBuffer, n);
            return NULL;  // The return value is meaningless for the free.
        }
        else // allocating
            return EASTLAlloc(*pAllocator, n);
    }


    /// allocate_memory
    ///
    /// This is a memory allocation dispatching function.
    /// To do: Make aligned and unaligned specializations.
    ///        Note that to do this we will need to use a class with a static
    ///        function instead of a standalone function like below.
    ///
    template <typename Allocator>
    void* allocate_memory(Allocator& a, size_t n, size_t alignment, size_t alignmentOffset)
    {
        if(alignment <= 8)
            return EASTLAlloc(a, n);
        return EASTLAllocAligned(a, n, alignment, alignmentOffset);
    }

} // namespace eastl





#ifndef EASTL_USER_DEFINED_ALLOCATOR // If the user hasn't declared that he has defined a different allocator implementation elsewhere...

#  ifdef _MSC_VER
#    pragma warning(push, 0)
//#    include <new>
#    pragma warning(pop)
#  else
//#    include <new>
#  endif

#if 1
		// If building a regular library and not building EASTL as a DLL...
        // It is expected that the application define the following
        // versions of operator new for the application. Either that or the
        // user needs to override the implementation of the allocator class.
        void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line);
        void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line);
#endif

    namespace eastl
    {
        inline allocator::allocator(const char* EASTL_NAME(pName))
        {
#  if EASTL_NAME_ENABLED
                mpName = pName ? pName : EASTL_ALLOCATOR_DEFAULT_NAME;
#  endif
        }


        inline allocator::allocator(const allocator& EASTL_NAME(alloc))
        {
#  if EASTL_NAME_ENABLED
                mpName = alloc.mpName;
#  endif
        }


        inline allocator::allocator(const allocator&, const char* EASTL_NAME(pName))
        {
#  if EASTL_NAME_ENABLED
                mpName = pName ? pName : EASTL_ALLOCATOR_DEFAULT_NAME;
#  endif
        }


        inline allocator& allocator::operator=(const allocator& EASTL_NAME(alloc))
        {
#  if EASTL_NAME_ENABLED
                mpName = alloc.mpName;
#  endif
            return *this;
        }


        inline const char* allocator::get_name() const
        {
#  if EASTL_NAME_ENABLED
                return mpName;
#  else
                return EASTL_ALLOCATOR_DEFAULT_NAME;
#  endif
        }


        inline void allocator::set_name(const char* EASTL_NAME(pName))
        {
#  if EASTL_NAME_ENABLED
                mpName = pName;
#  endif
        }


        inline void* allocator::allocate(size_t n, int flags)
        {
#  if EASTL_NAME_ENABLED
#    define pName mpName
#  else
#    define pName EASTL_ALLOCATOR_DEFAULT_NAME
#  endif

#  if EASTL_DLL
                // We currently have no support for implementing flags when 
                // using the C runtime library operator new function. The user 
                // can use SetDefaultAllocator to override the default allocator.
                (void)flags;
                return ::new char[n];
#  elif (EASTL_DEBUGPARAMS_LEVEL <= 0)
                return ::new((char*)0, flags, 0, (char*)0,        0) char[n];
#  elif (EASTL_DEBUGPARAMS_LEVEL == 1)
                return ::new(   pName, flags, 0, (char*)0,        0) char[n];
#  else
                return ::new(   pName, flags, 0, __FILE__, __LINE__) char[n];
#  endif
        }


        inline void* allocator::allocate(size_t n, size_t alignment, size_t offset, int flags)
        {
#  if EASTL_DLL
                // We have a problem here. We cannot support alignment, as we don't have access
                // to a memory allocator that can provide aligned memory. The C++ standard doesn't
                // recognize such a thing. The user will need to call SetDefaultAllocator to 
                // provide an alloator which supports alignment.
                EASTL_ASSERT(alignment <= 8); // 8 (sizeof(double)) is the standard alignment returned by operator new.
                (void)alignment; (void)offset; (void)flags;
                return new char[n];
#  elif (EASTL_DEBUGPARAMS_LEVEL <= 0)
                return ::new(alignment, offset, (char*)0, flags, 0, (char*)0,        0) char[n];
#  elif (EASTL_DEBUGPARAMS_LEVEL == 1)
                return ::new(alignment, offset,    pName, flags, 0, (char*)0,        0) char[n];
#  else
                return ::new(alignment, offset,    pName, flags, 0, __FILE__, __LINE__) char[n];
#  endif

#  undef pName  // See above for the definition of this.
        }


        inline void allocator::deallocate(void* p, size_t)
        {
            delete[] (char*)p;
        }


        inline bool operator==(const allocator&, const allocator&)
        {
            return true; // All allocators are considered equal, as they merely use global new/delete.
        }


        inline bool operator!=(const allocator&, const allocator&)
        {
            return false; // All allocators are considered equal, as they merely use global new/delete.
        }


    } // namespace eastl


#endif // EASTL_USER_DEFINED_ALLOCATOR


#ifdef _MSC_VER
#  pragma warning(pop)
#endif


#endif // Header include guard
















