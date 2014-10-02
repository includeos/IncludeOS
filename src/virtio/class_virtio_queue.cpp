#include <virtio/class_virtio.hpp>
#include <malloc.h>
#include <string.h>
#include <syscalls.hpp>

/** 
    Nested Virtio Queue class inside Virtio 
 */

#define ALIGN(x) (((x) + PAGE_SIZE) & ~PAGE_SIZE) 
unsigned Virtio::Queue::virtq_size(unsigned int qsz) 
{ 
  return ALIGN(sizeof(virtq_desc)*qsz + sizeof(u16)*(3 + qsz)) 
    + ALIGN(sizeof(u16)*3 + sizeof(virtq_used_elem)*qsz); 
}


void Virtio::Queue::init_queue(int size, void* buf){

  // The buffer starts with is an array of queue descriptors
  _queue.desc = (virtq_desc*)buf;
  printf("\t Queue desc  @ 0x%lx \n ",(long)_queue.desc);

  // The available buffer starts right after the queue descriptors
  _queue.avail = (virtq_avail*)((char*)buf + size*sizeof(virtq_desc));  
  printf("\t Queue avail @ 0x%lx \n ",(long)_queue.avail);

  // The used queue starts at the beginning of the next page
  // (This is  a formula from sanos - don't know why it works, but it does
  // align the used queue to the next page border)  
  _queue.used = (virtq_used*)(((uint32_t)&_queue.avail->ring[size] +
                                sizeof(uint16_t)+PAGESIZE-1) & ~(PAGESIZE -1));
  
  printf("\t Queue used  @ 0x%lx \n ",(long)_queue.used);
}

/** Constructor */
Virtio::Queue::Queue(uint16_t size)
  : _size(size),_size_bytes(virtq_size(size))
{
  //Allocate space for the queue and clear it out
  void* buffer = memalign(PAGE_SIZE,_size_bytes);
  //void* buffer = malloc(_size_bytes);
  if (!buffer) panic("Could not allocate space for Virtio::Queue");
  memset(buffer,0,_size_bytes);    
 
  printf(">>> Virtio Queue of size %i (%i bytes) initializing \n",
         _size,_size_bytes);  

  init_queue(size,buffer);
    
  printf(" >> Virtio Queue setup complete \n");    
}

void Virtio::Queue::notify(){
  printf("\t Queue notified. Checking buffers.... \n");
  printf("\t Used idx: %i, Avail idx: %i \n",
         _queue.used->idx, _queue.avail->idx );
}
