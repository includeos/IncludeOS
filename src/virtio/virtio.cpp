//
// virtio.c
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

#define KERNEL //In order to get func-defs in object.h

#include <os>
#include "virtio.h"

#include "../hw/pic.h"
#include "../hw/dev.h"
#include <malloc.h>
#include <string.h>



void virtio_dpc(void *arg)
{  
  struct virtio_device *vd = (struct virtio_device *) arg;
  struct virtio_queue *vq = vd->queues;
  
  // Notify all queues for device.
  while (vq)
  {
    if (vq->callback(vq) > 0) break;
    vq = vq->next;
  }
}

int virtio_handler(struct context *ctxt, void *arg)
{
  struct virtio_device *vd = (struct virtio_device *) arg;
  unsigned char isr;

  // Check if there is any work on this interrupt. Reading the interrupt
  // register also clears it.
  isr = inp(vd->iobase + VIRTIO_PCI_ISR);
  if (!isr) return 0;

  // Queue DPC to read the queues for the device.
  queue_irq_dpc(&vd->dpc, virtio_dpc, vd);
  eoi(vd->irq);
  return 1;
}

void virtio_get_config(struct virtio_device *vd, void *buf, int len)
{
  unsigned char *ptr = (unsigned char*)buf;
  int ioaddr = vd->iobase + VIRTIO_PCI_CONFIG;
  int i;
  for (i = 0; i < len; i++) *ptr++ = inp(ioaddr + i);
}

int virtio_device_init(struct virtio_device *vd, struct unit *unit, int features)
{
  // Get device resources.
  vd->unit = unit;
  vd->irq = get_unit_irq(unit);
  vd->iobase = get_unit_iobase(unit);
  vd->queues = NULL;

  // Reset device
  outp(vd->iobase + VIRTIO_PCI_STATUS, 0);

  // Indicate that driver has been found
  outp(vd->iobase + VIRTIO_PCI_STATUS, inp(vd->iobase + VIRTIO_PCI_STATUS) | VIRTIO_CONFIG_S_ACKNOWLEDGE | VIRTIO_CONFIG_S_DRIVER);

  // Negotiate features
  vd->features = inpd(vd->iobase + VIRTIO_PCI_HOST_FEATURES);
  vd->features &= features;
  outpd(vd->iobase + VIRTIO_PCI_GUEST_FEATURES, vd->features);

  // Enable interrupts
  register_interrupt(&vd->intr, IRQ2INTR(vd->irq), virtio_handler, vd);
  enable_irq(vd->irq);

  return 0;
}

void virtio_setup_complete(struct virtio_device *vd, int success)
{
  unsigned char status = success ? VIRTIO_CONFIG_S_DRIVER_OK : VIRTIO_CONFIG_S_FAILED;
  outp(vd->iobase + VIRTIO_PCI_STATUS, inp(vd->iobase + VIRTIO_PCI_STATUS) | status);
}

static unsigned int vring_size(unsigned int size)
{
  return ((sizeof(struct vring_desc) * size + sizeof(unsigned short) * (3 + size) + PAGESIZE - 1) & ~(PAGESIZE - 1)) +
         sizeof(unsigned short) * 3 + sizeof(struct vring_used_elem) * size;
}

static void vring_init(struct vring *vr, unsigned int size, void *p)
{
  vr->size = size;
  vr->desc = (vring_desc*)p;
  vr->avail = (struct vring_avail *) ((char *) p + size * sizeof(struct vring_desc));
  vr->used = (struct vring_used *) (((unsigned long) &vr->avail->ring[size] + sizeof(unsigned short) + PAGESIZE - 1) & ~(PAGESIZE - 1));
}

static int init_queue(struct virtio_queue *vq, int index, int size)
{
  unsigned int len;
  char *buffer;
  int i;
  
  // Initialize vring structure.
  len = vring_size(size);
  buffer = (char*)kmalloc(len);
  if (!buffer) return -ENOMEM;
  memset(buffer, 0, len);
  vring_init(&vq->vring, size, buffer);
  
  // Setup queue.
  vq->index = index;
  vq->last_used_idx = 0;
  vq->num_added = 0;

  // Put everything on the free list
  vq->num_free = size;
  vq->free_head = 0;
  for (i = 0; i < size - 1; i++) vq->vring.desc[i].next = i + 1;

  return 0;
}

int virtio_queue_init(struct virtio_queue *vq, struct virtio_device *vd, int index, virtio_callback_t callback)
{
  unsigned short size;
  int rc;

  // Select the queue
  outpw(vd->iobase + VIRTIO_PCI_QUEUE_SEL, index);

  // Check if queue is either not available or already active
  size = inpw(vd->iobase + VIRTIO_PCI_QUEUE_SIZE);
  if (!size || inpd(vd->iobase + VIRTIO_PCI_QUEUE_PFN)) return -ENOENT;

  // Initialize virtual queue
  rc = init_queue(vq, index, size);
  if (rc < 0) return rc;
  
  // Allocate space for callback data tokens
  vq->data = (void **) kmalloc(sizeof(void *) * size);
  if (!vq->data) return -ENOSPC;
  memset(vq->data, 0, sizeof(void *) * size);

  // Initialize buffer available event
  init_event(&vq->bufavail, 0, 1);

  // Attach queue to device
  vq->vd = vd;
  vq->next = vd->queues;
  vd->queues = vq;
  vq->callback = callback;

  // Activate the queue
  outpd(vd->iobase + VIRTIO_PCI_QUEUE_PFN, BTOP(virt2phys(vq->vring.desc)));

  return 0;
}

int virtio_queue_size(struct virtio_queue *vq)
{
  return vq->vring.size;
}

int virtio_enqueue(struct virtio_queue *vq, struct scatterlist sg[], unsigned int out, unsigned int in, void *data)
{
  int i, avail;
  int head, prev;

  // Wait for available buffers
  while (vq->num_free < out + in)
  {
    if (wait_for_object(&vq->bufavail, INFINITE) < 0) return -ENOSPC;
  }

  // Remove buffers from the free list
  vq->num_free -= out + in;
  head = vq->free_head;
  for (i = vq->free_head; out; i = vq->vring.desc[i].next, out--) 
  {
    vq->vring.desc[i].flags = VRING_DESC_F_NEXT;
    vq->vring.desc[i].addr = virt2phys(sg->data);
    vq->vring.desc[i].len = sg->size;
    prev = i;
    sg++;
  }
  for (; in; i = vq->vring.desc[i].next, in--) 
  {
    vq->vring.desc[i].flags = VRING_DESC_F_NEXT | VRING_DESC_F_WRITE;
    vq->vring.desc[i].addr = virt2phys(sg->data);
    vq->vring.desc[i].len = sg->size;
    prev = i;
    sg++;
  }
  
  // No continue on last buffer
  vq->vring.desc[prev].flags &= ~VRING_DESC_F_NEXT;

  // Update free pointer
  vq->free_head = i;

  // Set callback token
  vq->data[head] = data;

  // Put entry in available array, but do not update avail->idx until sync
  avail = (vq->vring.avail->idx + vq->num_added++) % vq->vring.size;
  vq->vring.avail->ring[avail] = head;

  // Notify about free buffers
  if (vq->num_free > 0) set_event(&vq->bufavail);
    
  return vq->num_free;
}

static void virtio_release(struct virtio_queue *vq, unsigned int head)
{
  unsigned int i;

  // Clear callback data token
  vq->data[head] = NULL;

  // Put buffers back on the free list; first find the end
  i = head;
  while (vq->vring.desc[i].flags & VRING_DESC_F_NEXT) 
  {
    i = vq->vring.desc[i].next;
    vq->num_free++;
  }
  vq->num_free++;

  // Add buffers back to free list
  vq->vring.desc[i].next = vq->free_head;
  vq->free_head = head;

  // Notify about free buffers
  if (vq->num_free > 0) set_event(&vq->bufavail);
}

void virtio_kick(struct virtio_queue *vq)
{
  // Make new entries available to host
  vq->vring.avail->idx += vq->num_added;
  vq->num_added = 0;

  // Notify host
  if (!(vq->vring.used->flags & VRING_USED_F_NO_NOTIFY))
  {
    outpw(vq->vd->iobase + VIRTIO_PCI_QUEUE_NOTIFY, vq->index);
  }
}

static int more_used(struct virtio_queue *vq)
{
  return vq->last_used_idx != vq->vring.used->idx;
}

void *virtio_dequeue(struct virtio_queue *vq, unsigned int *len)
{
  struct vring_used_elem *e;
  void *data;

  // Return NULL if there are no more completed buffers in the queue
  if (!more_used(vq)) return NULL;

  // Get next completed buffer
  e = &vq->vring.used->ring[vq->last_used_idx % vq->vring.size];
  *len = e->len;
  data = vq->data[e->id];
  
  // Release buffer
  virtio_release(vq, e->id);
  vq->last_used_idx++;

  return data;
}
