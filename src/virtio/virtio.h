//
// virtio.h
//
// Interface to virtual I/O devices (virtio)
//
// Copyright (C) 2011 Michael Ringgaard. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 
// 1. Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.  
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.  
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
// SUCH DAMAGE.
// 

#ifndef VIRTIO_H
#define VIRTIO_H

#include "object.h"

//
// PCI virtio I/O registers.
//

#define VIRTIO_PCI_HOST_FEATURES        0   // Features supported by the host
#define VIRTIO_PCI_GUEST_FEATURES       4   // Features activated by the guest
#define VIRTIO_PCI_QUEUE_PFN            8   // PFN for the currently selected queue
#define VIRTIO_PCI_QUEUE_SIZE           12  // Queue size for the currently selected queue
#define VIRTIO_PCI_QUEUE_SEL            14  // Queue selector
#define VIRTIO_PCI_QUEUE_NOTIFY         16  // Queue notifier
#define VIRTIO_PCI_STATUS               18  // Device status register
#define VIRTIO_PCI_ISR                  19  // Interrupt status register
#define VIRTIO_PCI_CONFIG               20  // Configuration data block

//
// PCI virtio status register bits
//

#define VIRTIO_CONFIG_S_ACKNOWLEDGE     1
#define VIRTIO_CONFIG_S_DRIVER          2
#define VIRTIO_CONFIG_S_DRIVER_OK       4
#define VIRTIO_CONFIG_S_FAILED          0x80

//
// Ring descriptor flags
//

#define VRING_DESC_F_NEXT       1   // Buffer continues via the next field
#define VRING_DESC_F_WRITE      2   // Buffer is write-only (otherwise read-only)
#define VRING_DESC_F_INDIRECT   4   // Buffer contains a list of buffer descriptors

//
// Ring descriptors for virtio
//

struct vring_desc 
{
  uint64_t addr;
  unsigned long len;
  unsigned short flags;
  unsigned short next;
};

#define VRING_AVAIL_F_NO_INTERRUPT 1

struct vring_avail
{
  unsigned short flags;
  unsigned short idx;
  unsigned short ring[0];
};

struct vring_used_elem 
{
  unsigned long id;
  unsigned long len;
};

#define VRING_USED_F_NO_NOTIFY 1

struct vring_used 
{
  unsigned short flags;
  unsigned short idx;
  struct vring_used_elem ring[0];
};

struct vring 
{
  unsigned int size;
  struct vring_desc *desc;
  struct vring_avail *avail;
  struct vring_used *used;
};

//
// Virtual queue
//

struct virtio_device;

typedef int (*virtio_callback_t)(struct virtio_queue *vq);

struct virtio_queue
{
  struct vring vring;           // Ring for storing queue data
  unsigned int num_free;        // Number of free buffers
  unsigned int free_head;       // Head of free buffer list
  unsigned int num_added;       // Elements added since last sync
  unsigned short last_used_idx; // Last used index seen
  unsigned short index;         // Queue index
  struct virtio_device *vd;     // Device this queue belongs to
  struct virtio_queue *next;    // Next queue for device
  struct event bufavail;        // Event for tracking free buffers
  virtio_callback_t callback;   // Callback for notifying about completion
  void **data;                  // Tokens for callbacks
};

//
// Virtual I/O device
//

struct virtio_device
{
  struct unit *unit;
  int irq;
  int iobase;
  struct interrupt intr;
  struct dpc dpc;
  unsigned long features;
  struct virtio_queue *queues;
};

//
// Scatter-gather buffer list element
//

struct scatterlist 
{
  void *data;
  int size;
};

//
// Virtual I/O API
//

krnlapi int virtio_device_init(struct virtio_device *vd, struct unit *unit, int features);
krnlapi void virtio_setup_complete(struct virtio_device *vd, int success);
krnlapi void virtio_get_config(struct virtio_device *vd, void *buf, int len);

krnlapi int virtio_queue_init(struct virtio_queue *vq, struct virtio_device *vd, int index, virtio_callback_t callback);
krnlapi int virtio_queue_size(struct virtio_queue *vq);
krnlapi int virtio_enqueue(struct virtio_queue *vq, struct scatterlist sg[], unsigned int out, unsigned int in, void *data);
krnlapi void virtio_kick(struct virtio_queue *vq);
krnlapi void *virtio_dequeue(struct virtio_queue *vq, unsigned int *len);

#endif
