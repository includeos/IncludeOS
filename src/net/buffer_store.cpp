#define DEBUG
#include <os>
#include <vector>
#include <malloc.h>
#include <net/buffer_store.hpp>

using namespace net;

BufferStore::BufferStore(int num, size_t bufsize, size_t offset ) :
  bufcount_(num), bufsize_(bufsize),  offset_(offset),
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

/** @todo Decide on a policy for how / when to increase storage */
void BufferStore::increaseStorage(){
  panic("Storage full!");
}

buffer BufferStore::get(){
  if (available_buffers_.empty())
    increaseStorage();
  auto buf = available_buffers_.back();
  available_buffers_.pop_back();  
  debug2("<BufferStore> Provisioned a buffer. %i buffers remaining\n", 
	available_buffers_.size());
  return buf;
}

void BufferStore::release(buffer b, size_t bufsize){
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

void BufferStore::release_offset(buffer b, size_t bufsize){
  debug2("<BufferStore> Trying to release %i + %i sized buffer  %p.  \n", bufsize, offset_, b);
  // Make sure the buffer comes from here. Otherwise, ignore it.
  if (address_is_from_pool(b) 
      and address_is_offset_bufstart(b)
      and bufsize == bufsize_ - offset_)
    {
      available_buffers_.push_back(b - offset_);
      
      debug("<BufferStore> Releasing %p. %i available buffers.\n", b, available_buffers_.size());
      return;
    }
  
  debug("<BufferStore> IGNORING buffer %p. It isn't mine.  \n", b);
}
