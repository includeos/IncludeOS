// Part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and  Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//#define DEBUG
//#define DEBUG2
#include <os>
#include <vector>
#include <malloc.h>
#include <net/buffer_store.hpp>
#include <cassert>

using namespace net;

BufferStore::BufferStore(int num, size_t bufsize, size_t device_offset ) :
  bufcount_(num), bufsize_(bufsize),  device_offset_(device_offset),
  pool_((buffer) memalign(PAGE_SIZE, num * bufsize))
{
  
  assert(pool_);
  
  debug ("<BufferStore> Creating buffer store of %i * %i bytes.",
	num, bufsize);
  
  
  for (buffer b = pool_; b < (buffer) pool_ + (num * bufsize); b += bufsize)
    available_buffers_.push_back(b);
  
  
  debug ("<BufferStore> I now have %i free buffers in range %p -> %p. \n",
	 available_buffers_.size(), pool_, pool_ + (bufcount_ * bufsize_));
  
}

BufferStore::~BufferStore(){
  free(pool_);
}

/** @todo : We (think we) want a list of pools, that we increase as needed.
 */
void BufferStore::increaseStorage(){
  panic("<BufferStore> Storage pool full! Don't know how to increase pool size yet.");
}

buffer BufferStore::get_raw_buffer(){
  if (available_buffers_.empty())
    increaseStorage();
  
  auto buf = available_buffers_.front();
  available_buffers_.pop_front();  
  
  debug2("<BufferStore> Provisioned a buffer. %i buffers remaining\n", 
	available_buffers_.size());
  
  return buf;
}

buffer BufferStore::get_offset_buffer(){  
  return get_raw_buffer() + device_offset_;
}

void BufferStore::release_raw_buffer(buffer b, size_t bufsize){
  debug2("<BufferStore> Trying to release %i sized buffer  %p. \n", bufsize, b);
  // Make sure the buffer comes from here. Otherwise, ignore it.
  if (address_is_from_pool(b) 
      and address_is_bufstart(b)
      and bufsize == bufsize_)
    {
      available_buffers_.push_back(b);
      debug("<BufferStore> Releasing %p. %i available buffers.\n", b, available_buffers_.size());
      return;
    }
  
  debug("<BufferStore> IGNORING buffer %p. It isn't mine.  \n", b);
}

void BufferStore::release_offset_buffer(buffer b, size_t bufsize){
  debug2("<BufferStore> Trying to release %i + %i sized buffer  %p.  \n", bufsize, device_offset_, b);
  // Make sure the buffer comes from here. Otherwise, ignore it.
  if (address_is_from_pool(b) 
      and address_is_offset_bufstart(b)
      and bufsize == bufsize_ - device_offset_)
    {
      available_buffers_.push_back(b - device_offset_);      
      debug("<BufferStore> Releasing %p. %i available buffers.\n", b, available_buffers_.size());
      return;
    }
  
  debug("<BufferStore> IGNORING buffer %p. It isn't mine.  \n", b);
}
