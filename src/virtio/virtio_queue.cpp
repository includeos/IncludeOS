// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
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

//#define DEBUG // Allow debug
//#define DEBUG2

#include <virtio/virtio.hpp>
#include <kernel/syscalls.hpp>
#include <hw/pci.hpp>
#include <malloc.h>
#include <string.h>
#include <assert.h>


/**
   Virtio Queue class, nested inside Virtio.
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
  debug("\t * Queue desc  @ 0x%lx \n ",(long)_queue.desc);

  // The available buffer starts right after the queue descriptors
  _queue.avail = (virtq_avail*)((char*)buf + size*sizeof(virtq_desc));
  debug("\t * Queue avail @ 0x%lx \n ",(long)_queue.avail);

  // The used queue starts at the beginning of the next page
  // (This is  a formula from sanos - don't know why it works, but it does
  // align the used queue to the next page border)
  _queue.used = (virtq_used*)(((uint32_t)&_queue.avail->ring[size] +
                               sizeof(uint16_t)+PAGESIZE-1) & ~(PAGESIZE -1));
  debug("\t * Queue used  @ 0x%lx \n ",(long)_queue.used);

}



/** A default handler doing nothing.

    It's here because we might not want to look at the data, e.g. for
    the VirtioNet TX-queue which will get used buffers in. */
int empty_handler(uint8_t* UNUSED(data),int UNUSED(size)) {
  debug("<Virtio::Queue> Empty handler. DROP! ");
  return -1;
};

/** Constructor */
Virtio::Queue::Queue(uint16_t size, uint16_t q_index, uint16_t iobase)
  : _size(size),_size_bytes(virtq_size(size)),_iobase(iobase),_num_free(size),
    _free_head(0), _num_added(0),_last_used_idx(0),_pci_index(q_index),
    _data_handler(delegate<int(uint8_t*,int)>(empty_handler))
{
  // Allocate page-aligned size and clear it
  void* buffer = memalign(PAGE_SIZE, _size_bytes);
  memset(buffer, 0, _size_bytes);

  debug(">>> Virtio Queue of size %i (%li bytes) initializing \n",
        _size,_size_bytes);
  init_queue(size,buffer);

  // Chain buffers
  debug("\t * Chaining buffers \n");
  for (int i=0; i<size; i++) _queue.desc[i].next = i+1;
  _queue.desc[size -1].next = 0;

  debug(" >> Virtio Queue setup complete. \n");
}


/** Ported more or less directly from SanOS. */
int Virtio::Queue::enqueue(scatterlist sg[], uint32_t out, uint32_t in, void* UNUSED(data)){

  uint16_t i,avail,head, prev = _free_head;


  while (_num_free < out + in){ // Queue is full (we think)
    //while( num_avail() >= _size) // Wait for Virtio
    printf("<Q %i>Buffer full (%i avail,"               \
           " used.idx: %i, avail.idx: %i )\n",
           _pci_index,num_avail(),
           _queue.used->idx,_queue.avail->idx
           );
    panic("Buffer full");
  }

  // Remove buffers from the free list
  _num_free -= out + in;
  head = _free_head;


  // (implicitly) Mark all outbound tokens as device-readable
  for (i = _free_head; out; i = _queue.desc[i].next, out--)
    {
      _queue.desc[i].flags = VIRTQ_DESC_F_NEXT;
      _queue.desc[i].addr = (uint64_t)sg->data;
      _queue.desc[i].len = sg->size;

      debug("<Q %i> Enqueueing outbound: index %i len %li, next %i\n",
            _pci_index,i,_queue.desc[i].len,_queue.desc[i].next);

      prev = i;
      sg++;
    }

  // Mark all inbound tokens as device-writable
  for (; in; i = _queue.desc[i].next, in--)
    {
      debug("<Q> Enqueuing inbound \n");
      _queue.desc[i].flags = VIRTQ_DESC_F_NEXT | VIRTQ_DESC_F_WRITE;
      _queue.desc[i].addr = (uint64_t)sg->data;
      _queue.desc[i].len = sg->size;
      prev = i;
      sg++;
    }

  // No continue on last buffer
  _queue.desc[prev].flags &= ~VIRTQ_DESC_F_NEXT;


  // Update free pointer
  _free_head = i;

  // Set callback token
  //vq->data[head] = data;

  // SanOS: Put entry in available array, but do not update avail->idx until sync
  avail = (_queue.avail->idx + _num_added++) % _size;
  _queue.avail->ring[avail] = head;
  debug("<Q %i> avail: %i \n",_pci_index,avail);

  // Notify about free buffers
  //if (_num_free > 0) set_event(&vq->bufavail);

  return _num_free;
}
void Virtio::Queue::enqueue(
    void*    out,
    uint32_t out_len,
    void*    in,
    uint32_t in_len)
{
  int total = (out) ? 1 : 0;
  total += (in) ? 1 : 0;
  
  printf("enqueue total: %d\n", total);
  
  if (_num_free < total)
  {
    // Queue is full (we think)
    printf("<Q %i>Buffer full (%i avail,"               \
           " used.idx: %i, avail.idx: %i )\n",
           _pci_index, num_avail(),
           _queue.used->idx,_queue.avail->idx
           );
    panic("Buffer full");
  }

  // Remove buffers from the free list
  _num_free -= total;
  // remember current head for later
  uint16_t head = _free_head;
  // the last buffer in queue
  virtq_desc* last = nullptr;

  // (implicitly) Mark all outbound tokens as device-readable
  if (out)
  {
    current().flags = VIRTQ_DESC_F_NEXT;
    current().addr = (intptr_t) out;
    current().len = out_len;
    
    printf("<Q %d> Enqueueing outbound: index %u len %u (actual: %u), next %d\n",
          _pci_index, head, current().len, out_len, current().next);
    
    last = &current();
    // go to next
    go_next();
  }

  // Mark all inbound tokens as device-writable
  if (in)
  {
    printf("<Q> Enqueuing inbound \n");
    current().flags = VIRTQ_DESC_F_NEXT | VIRTQ_DESC_F_WRITE;
    current().addr = (intptr_t) in;
    current().len = in_len;
    
    last = &current();
    // go to next
    go_next();
  }

  // No continue on last buffer
  last->flags &= ~VIRTQ_DESC_F_NEXT;

  // SanOS: Put entry in available array, but do not update avail->idx until sync
  uint16_t avail = (_queue.avail->idx + _num_added++) % _size;
  _queue.avail->ring[avail] = head;
  debug("<Q%u> avail: %u\n", _pci_index, avail);
}
void Virtio::Queue::enqueue(void* data, uint32_t len, bool out, bool last)
{
  if (_num_free < 1)
  {
    // Queue is full (we think)
    printf("<Q %i>Buffer full (%i avail,"               \
           " used.idx: %i, avail.idx: %i )\n",
           _pci_index, num_avail(),
           _queue.used->idx,_queue.avail->idx
           );
    panic("Buffer full");
  }

  // Remove buffers from the free list
  _num_free -= 1;
  // remember current head for later
  uint16_t head = _free_head;
  // No continue on last buffer
  uint16_t flags = last ? 0 : VIRTQ_DESC_F_NEXT;
  // WRITE for inbound
  current().flags = flags | (out ? 0 : VIRTQ_DESC_F_WRITE);
  current().addr = (intptr_t) data;
  current().len = len;
  
  if (out)
  printf("<Q %d> Enqueueing outbound: %p index %u len %u (actual: %u), next %d\n",
        _pci_index, data, head, current().len, len, current().next);
  else
  printf("<Q %d> Enqueueing inbound: %p index %u len %u (actual: %u), next %d\n",
        _pci_index, data, head, current().len, len, current().next);
  
  // go to next in ring
  go_next();
  
  // SanOS: Put entry in available array, but do not update avail->idx until sync
  uint16_t avail = (_queue.avail->idx + _num_added++) % _size;
  _queue.avail->ring[avail] = head;
}
void* Virtio::Queue::dequeue(uint32_t& len)
{
  // Return NULL if there are no more completed buffers in the queue
  if (_last_used_idx == _queue.used->idx)
  {
    debug("<Q %i> Can't dequeue - no used buffers \n",_pci_index);
    return nullptr;
  }

  // Get next completed buffer
  auto& e = _queue.used->ring[_last_used_idx % _size];

  printf("<Q %i> Releasing token %u. Len: %u\n",_pci_index, e.id, e.len);
  void* data = (void*) _queue.desc[e.id].addr;
  len = e.len;

  // Release buffer
  release(e.id);
  _last_used_idx++;

  return data;
}

void Virtio::Queue::release(uint32_t head)
{
  // Mark queue element "head" as free (the whole token chain)
  uint32_t i = head;

  //It's at least one token...
  _num_free++;

  //...possibly with a tail

  while (_queue.desc[i].flags & VIRTQ_DESC_F_NEXT)
    {
      i = _queue.desc[i].next;
      _num_free++;
    }

  // Add buffers back to free list
  _queue.desc[i].next = _free_head;
  _free_head = head;

  // What happens here?
  debug2("<Q %i> desc[%i].next : %i \n",_pci_index,i,_queue.desc[i].next);
}

uint8_t* Virtio::Queue::dequeue(uint32_t* len){

  // Return NULL if there are no more completed buffers in the queue
  if (_last_used_idx == _queue.used->idx){
    debug("<Q %i> Can't dequeue - no used buffers \n",_pci_index);
    return NULL;
  }

  // Get next completed buffer
  auto* e = &_queue.used->ring[_last_used_idx % _size];
  *len = e->len;

  debug2("<Q %i> Releasing token %li. Len: %li\n",_pci_index,e->id, e->len);
  uint8_t* data = (uint8_t*)_queue.desc[e->id].addr;

  // Release buffer
  release(e->id);
  _last_used_idx++;

  return data;
}

void Virtio::Queue::set_data_handler(delegate<int(uint8_t* data,int len)> del){
  _data_handler=del;
}

void Virtio::Queue::disable_interrupts(){
  _queue.avail->flags |= (1 << VIRTQ_AVAIL_F_NO_INTERRUPT);
}

void Virtio::Queue::enable_interrupts(){
  _queue.avail->flags &= ~(1 << VIRTQ_AVAIL_F_NO_INTERRUPT);
}

void Virtio::Queue::kick(){
  //__sync_synchronize ();

  // Atomically increment (maybe not necessary?)
  //__sync_add_and_fetch(&(_queue.avail->idx),_num_added);
  _queue.avail->idx += _num_added;
  //__sync_synchronize ();

  _num_added = 0;


  if (!(_queue.used->flags & VIRTQ_USED_F_NO_NOTIFY)){
    debug("<Queue %i> Kicking virtio. Iobase 0x%x \n",
          _pci_index, _iobase);
    //hw::outpw(_iobase + VIRTIO_PCI_QUEUE_SEL, _pci_index);
    hw::outpw(_iobase + VIRTIO_PCI_QUEUE_NOTIFY , _pci_index);
  }else{
    debug("<VirtioQueue>Virtio device says we can't kick!");
  }
}
