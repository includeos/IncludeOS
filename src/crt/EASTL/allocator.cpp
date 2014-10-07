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
// EASTL/allocator.cpp
//
// Copyright (c) 2005, Electronic Arts. All rights reserved.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////


#include <EASTL/internal/config.h>
#include <EASTL/allocator.h>


///////////////////////////////////////////////////////////////////////////////
// ReadMe
//
// This file implements the default application allocator. 
// You can replace this allocator.cpp file with a different one,
// you can define EASTL_USER_DEFINED_ALLOCATOR below to ignore this file,
// or you can modify the EASTL config.h file to redefine how allocators work.
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_USER_DEFINED_ALLOCATOR // If the user hasn't declared that he has defined an allocator implementation elsewhere...

    namespace eastl
    {

        /// gDefaultAllocator
        /// Default global allocator instance. 
        EASTL_API allocator   gDefaultAllocator;
        EASTL_API allocator* gpDefaultAllocator = &gDefaultAllocator;

        EASTL_API allocator* GetDefaultAllocator()
        {
            return gpDefaultAllocator;
        }

        EASTL_API allocator* SetDefaultAllocator(allocator* pAllocator)
        {
            allocator* const pPrevAllocator = gpDefaultAllocator;
            gpDefaultAllocator = pAllocator;
            return pPrevAllocator;
        }

    } // namespace eastl


#endif // EASTL_USER_DEFINED_ALLOCATOR











