#include <virtio/class_virtio.hpp>
#include <malloc.h>
#include <string.h>
#include <syscalls.hpp>

/** 
    Nested Virtio Queue class inside Virtio 
 */

unsigned Virtio::Queue::virtq_size(unsigned int qsz) 
{ 
  return ALIGN(sizeof(struct virtq_desc)*qsz + sizeof(u16)*(3 + qsz)) 
    + ALIGN(sizeof(u16)*3 + sizeof(struct virtq_used_elem)*qsz); 
}


void Virtio::Queue::init_queue(int size){
  _queue->desc = (virtq_desc*)_queue;
}

/** Constructor */
Virtio::Queue::Queue(int size)
  : _size(virtq_size(size))
{
  //Allocate space for the queue and clear it out
  _queue=(virtq*)malloc(_size);
  if (!_queue) panic("Could not allocate space for Virtio::Queue");
  memset((char*)_queue,0,_size);
  
  init_queue(size);
  
  
  printf("Virtio Queue of size %i (%i bytes) initializing \n",size,_size);
  
}
