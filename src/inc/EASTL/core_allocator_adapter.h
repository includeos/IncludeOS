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
// core_allocator_adapter.h
// 
// Copyright (c) 2007, Electronic Arts. All rights reserved.
// Maintained by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Implements an EASTL allocator that uses an ICoreAllocator.
// However, this header file is not dependent on ICoreAllocator or its package.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_CORE_ALLOCATOR_ADAPTER_H
#define EASTL_CORE_ALLOCATOR_ADAPTER_H


#include <EASTL/allocator.h>


namespace EA
{
    namespace Allocator
    {
        /// CoreAllocatorAdapter
        ///
        /// Implements the EASTL allocator interface.
        /// Allocates memory from an instance of ICoreAllocator.
        ///
        /// Example usage:
        ///     eastl::list<Widget, CoreAllocatorAdapter<ICoreAllocator> > widgetList("UI/WidgetList", pSomeCoreAllocator);
        ///     widgetList.push_back(Widget());
        ///
        /// Example usage:
        ///    // Note that the CoreAllocator is declared before and thus destroyed after the widget list.
        ///    typedef CoreAllocatorAdapter<ICoreAllocator> EASTLCoreAllocator;
        ///    typedef eastl::list<Widget, EASTLCoreAllocator> WidgetList;
        ///    CoreAllocatorFixed<WidgetList::node_type> widgetCoreAllocator(pFixedAllocatorForWidgetListValueType);
        ///    WidgetList widgetList(EASTLCoreAllocator("UI/WidgetList", &widgetCoreAllocator));
        ///
        template<class AllocatorType>
        class CoreAllocatorAdapter
        {
        public:
            typedef CoreAllocatorAdapter<AllocatorType> this_type;

        public:
            CoreAllocatorAdapter(const char* pName = EASTL_NAME_VAL(EASTL_ALLOCATOR_DEFAULT_NAME));
            CoreAllocatorAdapter(const char* pName, AllocatorType* pAllocator);
            CoreAllocatorAdapter(const char* pName, AllocatorType* pAllocator, int flags);
            CoreAllocatorAdapter(const CoreAllocatorAdapter& x);
            CoreAllocatorAdapter(const CoreAllocatorAdapter& x, const char* pName);

            CoreAllocatorAdapter& operator=(const CoreAllocatorAdapter& x);

            void* allocate(size_t n, int flags = 0);
            void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0);
            void  deallocate(void* p, size_t n);

            AllocatorType* get_allocator() const;
            void           set_allocator(AllocatorType* pAllocator);

            int  get_flags() const;
            void set_flags(int flags);

            const char* get_name() const;
            void        set_name(const char* pName);

        public: // Public because otherwise VC++ generates (possibly invalid) warnings about inline friend template specializations.
            AllocatorType* mpCoreAllocator;
            int            mnFlags;    // Allocation flags. See ICoreAllocator/AllocFlags.

            #if EASTL_NAME_ENABLED
                const char* mpName; // Debug name, used to track memory.
            #endif
        };

        template<class AllocatorType>
        bool operator==(const CoreAllocatorAdapter<AllocatorType>& a, const CoreAllocatorAdapter<AllocatorType>& b);

        template<class AllocatorType>
        bool operator!=(const CoreAllocatorAdapter<AllocatorType>& a, const CoreAllocatorAdapter<AllocatorType>& b);



        /// EASTLICoreAllocator
        ///
        /// Provides a standardized typedef for ICoreAllocator;
        /// 
        /// Example usage:
        ///     eastl::list<Widget, EASTLICoreAllocator> widgetList("UI/WidgetList", pSomeCoreAllocator);
        ///     widgetList.push_back(Widget());
        ///
        class ICoreAllocator;
        typedef CoreAllocatorAdapter<ICoreAllocator> EASTLICoreAllocator;


    } // namespace Allocator

} // namespace EA





///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace EA
{
    namespace Allocator
    {

        template<class AllocatorType>
        inline CoreAllocatorAdapter<AllocatorType>::CoreAllocatorAdapter(const char* EASTL_NAME(pName))
            : mpCoreAllocator(AllocatorType::GetDefaultAllocator()), mnFlags(0)
        {
            #if EASTL_NAME_ENABLED
                mpName = pName ? pName : EASTL_ALLOCATOR_DEFAULT_NAME;
            #endif
        }

        template<class AllocatorType>
        inline CoreAllocatorAdapter<AllocatorType>::CoreAllocatorAdapter(const char* EASTL_NAME(pName), AllocatorType* pCoreAllocator)
            : mpCoreAllocator(pCoreAllocator), mnFlags(0)
        {
            #if EASTL_NAME_ENABLED
                mpName = pName ? pName : EASTL_ALLOCATOR_DEFAULT_NAME;
            #endif
        }

        template<class AllocatorType>
        inline CoreAllocatorAdapter<AllocatorType>::CoreAllocatorAdapter(const char* EASTL_NAME(pName), AllocatorType* pCoreAllocator, int flags)
            : mpCoreAllocator(pCoreAllocator), mnFlags(flags)
        {
            #if EASTL_NAME_ENABLED
                mpName = pName ? pName : EASTL_ALLOCATOR_DEFAULT_NAME;
            #endif
        }

        template<class AllocatorType>
        inline CoreAllocatorAdapter<AllocatorType>::CoreAllocatorAdapter(const CoreAllocatorAdapter& x)
            : mpCoreAllocator(x.mpCoreAllocator), mnFlags(x.mnFlags)
        {
            #if EASTL_NAME_ENABLED
                mpName = x.mpName;
            #endif
        }

        template<class AllocatorType>
        inline CoreAllocatorAdapter<AllocatorType>::CoreAllocatorAdapter(const CoreAllocatorAdapter& x, const char* EASTL_NAME(pName))
            : mpCoreAllocator(x.mpCoreAllocator), mnFlags(x.mnFlags)
        {
            #if EASTL_NAME_ENABLED
                mpName = pName ? pName : EASTL_ALLOCATOR_DEFAULT_NAME;
            #endif
        }

        template<class AllocatorType>
        inline CoreAllocatorAdapter<AllocatorType>& CoreAllocatorAdapter<AllocatorType>::operator=(const CoreAllocatorAdapter& x)
        {
            // In order to be consistent with EASTL's allocator implementation, 
            // we don't copy the name from the source object.
            mpCoreAllocator = x.mpCoreAllocator;
            mnFlags         = x.mnFlags;
            return *this;
        }

        template<class AllocatorType>
        inline void* CoreAllocatorAdapter<AllocatorType>::allocate(size_t n, int /*flags*/)
        {
            // It turns out that EASTL itself doesn't use the flags parameter, 
            // whereas the user here might well want to specify a flags 
            // parameter. So we use ours instead of the one passed in.
            return mpCoreAllocator->Alloc(n, EASTL_NAME_VAL(mpName), (unsigned)mnFlags);
        }

        template<class AllocatorType>
        inline void* CoreAllocatorAdapter<AllocatorType>::allocate(size_t n, size_t alignment, size_t offset, int /*flags*/)
        {
            // It turns out that EASTL itself doesn't use the flags parameter, 
            // whereas the user here might well want to specify a flags 
            // parameter. So we use ours instead of the one passed in.
            return mpCoreAllocator->Alloc(n, EASTL_NAME_VAL(mpName), (unsigned)mnFlags, (unsigned)alignment, (unsigned)offset);
        }

        template<class AllocatorType>
        inline void CoreAllocatorAdapter<AllocatorType>::deallocate(void* p, size_t n)
        {
            return mpCoreAllocator->Free(p, n);
        }

        template<class AllocatorType>
        inline AllocatorType* CoreAllocatorAdapter<AllocatorType>::get_allocator() const
        {
            return mpCoreAllocator;
        }

        template<class AllocatorType>
        inline void CoreAllocatorAdapter<AllocatorType>::set_allocator(AllocatorType* pAllocator)
        {
            mpCoreAllocator = pAllocator;
        }

        template<class AllocatorType>
        inline int CoreAllocatorAdapter<AllocatorType>::get_flags() const
        {
            return mnFlags;
        }

        template<class AllocatorType>
        inline void CoreAllocatorAdapter<AllocatorType>::set_flags(int flags)
        {
            mnFlags = flags;
        }

        template<class AllocatorType>
        inline const char* CoreAllocatorAdapter<AllocatorType>::get_name() const
        {
            #if EASTL_NAME_ENABLED
                return mpName;
            #else
                return EASTL_ALLOCATOR_DEFAULT_NAME;
            #endif
        }

        template<class AllocatorType>
        inline void CoreAllocatorAdapter<AllocatorType>::set_name(const char* pName)
        {
            #if EASTL_NAME_ENABLED
                mpName = pName;
            #else
                (void)pName;
            #endif
        }



        template<class AllocatorType>
        inline bool operator==(const CoreAllocatorAdapter<AllocatorType>& a, const CoreAllocatorAdapter<AllocatorType>& b)
        {
            return (a.mpCoreAllocator == b.mpCoreAllocator) &&
                   (a.mnFlags         == b.mnFlags);
        }

        template<class AllocatorType>
        inline bool operator!=(const CoreAllocatorAdapter<AllocatorType>& a, const CoreAllocatorAdapter<AllocatorType>& b)
        {
            return (a.mpCoreAllocator != b.mpCoreAllocator) ||
                   (a.mnFlags         != b.mnFlags);
        }


    } // namespace Allocator

} // namespace EA


#endif // Header include guard








