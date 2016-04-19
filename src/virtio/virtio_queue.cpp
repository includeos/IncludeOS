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

#define DEBUG // Allow debug
//#define DEBUG2

#include <os>
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

  // The buffer starts with is an array of queue descriptors (i.e. tokens)
  _queue.desc = (virtq_desc*)buf;
  debug("\t * Queue desc  @ 0x%lx \n ",(long)_queue.desc);

  // The available buffer starts right after the queue descriptors
  _queue.avail = (virtq_avail*)((char*)buf + size*sizeof(virtq_desc));
  debug("\t * Queue avail @ 0x%lx \n ",(long)_queue.avail);

  // The used queue starts at the beginning of the next page
  // (This is  a formula from sanos - don't know why it works, but it does
  // align the used queue to the next page border)
  _queue.used = (virtq_used*)(((uint32_t)&_queue.avail->ring[size] +
                               sizeof(uint16_t)+OS::page_size()-1) & ~(OS::page_size() -1));
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
  : _size(size),_size_bytes(virtq_size(size)),_iobase(iobase),
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
int Virtio::Queue::enqueue(gsl::span<Token> buffers){
  debug ("Enqueuing %i tokens \n", buffers.size());
  Expects(_free_head >= 0);
  Expects(_free_head < size());
  Expects(_num_added + 1 > _num_added);

  uint16_t last = _free_head;
  uint16_t first = _free_head;
  // Place each buffer in a token
  for( auto buf : buffers )  {
    debug (" buf @ %p \n", buffers.data());

    // Set read / write flags
    _queue.desc[_free_head].flags =
      buf.direction() ? VIRTQ_DESC_F_NEXT : VIRTQ_DESC_F_NEXT | VIRTQ_DESC_F_WRITE;

    // Assign raw buffer
    _queue.desc[_free_head].addr = (uint64_t) buf.data();
    _queue.desc[_free_head].len = buf.size();

    last = _free_head;
    _free_head ++;
  }

  // No continue on last buffer
  _queue.desc[last].flags &= ~VIRTQ_DESC_F_NEXT;

  // Place the head of this current chain in the avail ring
  uint16_t avail_index = (_queue.avail->idx + _num_added++) % _size;
  _queue.avail->ring[avail_index] = first;

  debug("<Q %i> avail_index: %i size: %i, _free_head %i \n",
        _pci_index, avail_index, size(), _free_head );

  Ensures(_free_head <= size());
  return buffers.size();
}

void Virtio::Queue::release(uint32_t head)
{
  // Mark queue element "head" as free (the whole token chain)
  uint32_t i = head;

  while (_queue.desc[i].flags & VIRTQ_DESC_F_NEXT)
    {
      i = _queue.desc[i].next;
    }

  // Add buffers back to free list
  _queue.desc[i].next = _free_head;
  _free_head = head;

  // What happens here?
  debug("<Q %i> desc[%i].next : %i \n",_pci_index, i ,_queue.desc[i].next);
}

Virtio::Token Virtio::Queue::dequeue(){

  // Return NULL if there are no more completed buffers in the queue
  if (_last_used_idx == _queue.used->idx){
    debug("<Q %i> Can't dequeue - no used buffers \n",_pci_index);
    return {{nullptr, 0}, Token::IN};

  }

  //debug("<Q%i> Dequeueing  last_used index %i ",_pci_index, _last_used_idx);

  // Get next completed buffer
  auto* e = &_queue.used->ring[_last_used_idx % _size];

  debug("<Q %i> Releasing token @%p, nr. %i Len: %i\n",_pci_index, e, e->id, e->len);

  // Release buffer
  release(e->id);
  _last_used_idx++;

  Token token1 {{(uint8_t*) _queue.desc[e->id].addr, (gsl::span<char>::size_type) e->len }, Token::IN};

  return token1;
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

  update_avail_idx();

  if (!(_queue.used->flags & VIRTQ_USED_F_NO_NOTIFY)){
    debug("<Queue %i> Kicking virtio. Iobase 0x%x \n",
          _pci_index, _iobase);
    //hw::outpw(_iobase + VIRTIO_PCI_QUEUE_SEL, _pci_index);
    hw::outpw(_iobase + VIRTIO_PCI_QUEUE_NOTIFY , _pci_index);
  }else{
    debug("<VirtioQueue>Virtio device says we can't kick!");
  }
}
