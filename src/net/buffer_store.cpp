#define DEBUG
#include <os>
#include <vector>
#include <malloc.h>
#include <net/buffer_store.hpp>

using namespace net;

BufferStore::BufferStore(int num, size_t bufsize) :
  bufcount_(num), bufsize_(bufsize),
  pool_((buffer) memalign(PAGE_SIZE, num * bufsize))
{
  
  debug ("<BufferStore> Creating buffer store of %i * %i bytes.",
	num, bufsize);
  
    
  for (buffer b = pool_; b < (buffer) pool_ + (num * bufsize); b += bufsize)
    available_buffers_.push_back(b);

    
  debug ("<BufferStore> I now have %i free buffers. \n",
	 available_buffers_.size());
  
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
  debug("<BufferStore> Provisioned a buffer. %i buffers remaining\n", 
	available_buffers_.size());
  return buf;
}


void BufferStore::release(buffer b){
  // Make sure the buffer comes from here. Otherwise, ignore it.
  if (b >= pool_ and
      b < pool_ + (bufcount_ * bufsize_) and
      (b - pool_) % bufsize_ == 0) 
    {
      available_buffers_.push_back(b);
      debug("<BufferStore> Releasing %p. %i available buffers.\n", b, available_buffers_.size());
      return;
    }
  
  debug("<BufferStore> Buffer %p isn't mine. Ignoring \n", b);
}
