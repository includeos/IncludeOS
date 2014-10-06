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
// EASTL/fixed_pool.cpp
//
// Copyright (c) 2005, Electronic Arts. All rights reserved.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////



#include <EASTL/internal/fixed_pool.h>
#include <EASTL/fixed_allocator.h>



namespace eastl
{


    void fixed_pool_base::init(void* pMemory, size_t memorySize, size_t nodeSize,
                               size_t alignment, size_t /*alignmentOffset*/)
    {
        // To do: Support alignmentOffset.

        #if EASTL_FIXED_SIZE_TRACKING_ENABLED
            mnCurrentSize = 0;
            mnPeakSize    = 0;
        #endif

        if(pMemory)
        {
            // Assert that alignment is a power of 2 value (e.g. 1, 2, 4, 8, 16, etc.)
            EASTL_ASSERT((alignment & (alignment - 1)) == 0);

            // Make sure alignment is a valid value.
            if(alignment < 1)
                alignment = 1;

            mpNext      = (Link*)(((uintptr_t)pMemory + (alignment - 1)) & ~(alignment - 1));
            memorySize -= (uintptr_t)mpNext - (uintptr_t)pMemory;
            pMemory     = mpNext;

            // The node size must be at least as big as a Link, which itself is sizeof(void*).
            if(nodeSize < sizeof(Link))
                nodeSize = ((sizeof(Link) + (alignment - 1))) & ~(alignment - 1);

            // If the user passed in a memory size that wasn't a multiple of the node size,
            // we need to chop down the memory size so that the last node is not a whole node.
            memorySize = (memorySize / nodeSize) * nodeSize;

            mpCapacity = (Link*)((uintptr_t)pMemory + memorySize);
            mpHead     = NULL;
            mnNodeSize = nodeSize;
        }
    }


} // namespace eastl

















