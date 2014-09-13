//
// virtioblk.c
//
// Block device driver for virtio
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

#include <os/krnl.h>

//
// Feature bits
//

#define VIRTIO_BLK_F_BARRIER    (1 << 0)  // Does host support barriers?
#define VIRTIO_BLK_F_SIZE_MAX   (1 << 1)  // Indicates maximum segment size
#define VIRTIO_BLK_F_SEG_MAX    (1 << 2)  // Indicates maximum # of segments
#define VIRTIO_BLK_F_GEOMETRY   (1 << 4)  // Geometry available
#define VIRTIO_BLK_F_RO         (1 << 5)  // Disk is read-only
#define VIRTIO_BLK_F_BLK_SIZE   (1 << 6)  // Block size of disk is available
#define VIRTIO_BLK_F_SCSI       (1 << 7)  // Supports scsi command passthru
#define VIRTIO_BLK_F_FLUSH      (1 << 9)  // Cache flush command support
#define VIRTIO_BLK_F_TOPOLOGY   (1 << 10) // Topology information is available

//
// Virtual disk configuration block
//

struct virtio_blk_config
{
  unsigned __int64 capacity;         // The capacity (in 512-byte sectors)
  unsigned long size_max;            // The maximum segment size
  unsigned long seg_max;             // The maximum number of segments
  struct virtio_blk_geometry {       // Device geometry
    unsigned short cylinders;
    unsigned char heads;
    unsigned char sectors;
  } geometry;
  unsigned long blk_size;            // Block size of device
  unsigned char physical_block_exp;  // Exponent for physical block per logical block
  unsigned char alignment_offset;    // Alignment offset in logical blocks
  unsigned short min_io_size;        // Minimum I/O size without performance penalty in logical blocks
  unsigned long opt_io_size;         // Optimal sustained I/O size in logical blocks
};

//
// Virtual block device request header
//

#define VIRTIO_BLK_T_IN        0
#define VIRTIO_BLK_T_OUT       1
#define VIRTIO_BLK_T_SCSI_CMD  2
#define VIRTIO_BLK_T_FLUSH     4
#define VIRTIO_BLK_T_GET_ID    8

struct virtio_blk_outhdr 
{
  unsigned long type;
  unsigned long ioprio;
  unsigned __int64 sector;
};

//
// Virtual block device satus codes
//

#define VIRTIO_BLK_S_OK      0
#define VIRTIO_BLK_S_IOERR   1
#define VIRTIO_BLK_S_UNSUPP  2

//
// Virtual disk device data
//

struct virtioblk
{
  struct virtio_device vd;
  struct virtio_blk_config config;
  struct virtio_queue vq;
  int capacity;
  dev_t devno;
};

//
// Virtual block device request
//

struct virtioblk_request
{
  struct virtio_blk_outhdr hdr;
  unsigned char status;
  struct thread *thread;
  unsigned int size;
};

static void virtioblk_setup_request(struct virtioblk_request *req, struct scatterlist *sg, void *buffer, int size)
{
  req->status = 0;
  req->thread = self();
  sg[0].data = &req->hdr;
  sg[0].size = sizeof(req->hdr);
  sg[1].data = buffer;
  sg[1].size = size;
  sg[2].data = &req->status;
  sg[2].size = sizeof(req->status);
}

static int virtioblk_ioctl(struct dev *dev, int cmd, void *args, size_t size)
{
  struct virtioblk *vblk = (struct virtioblk *) dev->privdata;
  struct geometry *geom;

  switch (cmd)
  {
    case IOCTL_GETDEVSIZE:
      return vblk->capacity;

    case IOCTL_GETBLKSIZE:
      return SECTORSIZE;

    case IOCTL_GETGEOMETRY:
      if (!args || size != sizeof(struct geometry)) return -EINVAL;
      if (!(vblk->vd.features & VIRTIO_BLK_F_GEOMETRY)) return -ENOSYS;
      geom = (struct geometry *) args;
      geom->cyls = vblk->config.geometry.cylinders;
      geom->heads = vblk->config.geometry.heads;
      geom->spt = vblk->config.geometry.sectors;
      geom->sectorsize = SECTORSIZE;
      geom->sectors = vblk->capacity;
      return 0;
  }

  return -ENOSYS;
}

static int virtioblk_read(struct dev *dev, void *buffer, size_t count, blkno_t blkno, int flags)
{
  struct virtioblk *vblk = (struct virtioblk *) dev->privdata;
  struct virtioblk_request req;
  struct scatterlist sg[3];
  int rc;

  // Setup read request
  virtioblk_setup_request(&req, sg, buffer, count);
  req.hdr.type = VIRTIO_BLK_T_IN;
  req.hdr.ioprio = 0;
  req.hdr.sector = blkno;
  
  // Issue request
  rc = virtio_enqueue(&vblk->vq, sg, 1, 2, &req);
  if (rc < 0) return rc;
  virtio_kick(&vblk->vq);
  
  // Wait for request to complete
  enter_wait(THREAD_WAIT_DEVIO);

  // Check status code
  switch (req.status)
  {
    case VIRTIO_BLK_S_OK: rc = req.size - 1; break;
    case VIRTIO_BLK_S_UNSUPP: rc = -ENODEV; break;
    case VIRTIO_BLK_S_IOERR: rc = -EIO; break;
    default: rc = -EUNKNOWN; break;
  }

  return rc;
}

static int virtioblk_write(struct dev *dev, void *buffer, size_t count, blkno_t blkno, int flags)
{
  struct virtioblk *vblk = (struct virtioblk *) dev->privdata;
  struct virtioblk_request req;
  struct scatterlist sg[3];
  int rc;

  // Setup write request
  virtioblk_setup_request(&req, sg, buffer, count);
  req.hdr.type = VIRTIO_BLK_T_OUT;
  req.hdr.ioprio = 0;
  req.hdr.sector = blkno;
  
  // Issue request
  rc = virtio_enqueue(&vblk->vq, sg, 2, 1, &req);
  if (rc < 0) return rc;
  virtio_kick(&vblk->vq);
  
  // Wait for request to complete
  enter_wait(THREAD_WAIT_DEVIO);

  // Check status code
  switch (req.status)
  {
    case VIRTIO_BLK_S_OK: rc = req.size - 1; break;
    case VIRTIO_BLK_S_UNSUPP: rc = -ENODEV; break;
    case VIRTIO_BLK_S_IOERR: rc = -EIO; break;
    default: rc = -EUNKNOWN; break;
  }

  return rc;
}

static int virtioblk_callback(struct virtio_queue *vq)
{
  struct virtioblk_request *req;
  unsigned int len;

  while ((req = virtio_dequeue(vq, &len)) != NULL)
  {
    req->size = len;
    mark_thread_ready(req->thread, 1, 2);
  }
  
  return 0;
}

struct driver virtioblk_driver =
{
  "virtioblk",
  DEV_TYPE_BLOCK,
  virtioblk_ioctl,
  virtioblk_read,
  virtioblk_write
};

static int install_virtioblk(struct unit *unit)
{
  struct virtioblk *vblk;
  int rc;

  // Setup unit information
  if (!unit) return -ENOSYS;
  unit->vendorname = "VIRTIO";
  unit->productname = "VIRTIO Virtual Block Device";
  
  // Allocate memory for device
  vblk = kmalloc(sizeof(struct virtioblk));
  if (vblk == NULL) return -ENOMEM;
  memset(vblk, 0, sizeof(struct virtioblk));

  // Initialize virtual device
  rc = virtio_device_init(&vblk->vd, unit, VIRTIO_BLK_F_SEG_MAX | VIRTIO_BLK_F_SIZE_MAX | VIRTIO_BLK_F_GEOMETRY | VIRTIO_BLK_F_RO | VIRTIO_BLK_F_BLK_SIZE | VIRTIO_BLK_F_FLUSH);
  if (rc < 0) return rc;
  
  // Get block device configuration
  virtio_get_config(&vblk->vd, &vblk->config, sizeof(vblk->config));
  if ((vblk->config.capacity & ~0x7FFFFFFF))
    vblk->capacity = 0x7FFFFFFF;
  else
    vblk->capacity = (int) vblk->config.capacity;
  
  // Initialize queue for disk requests
  rc = virtio_queue_init(&vblk->vq, &vblk->vd, 0, virtioblk_callback);
  if (rc < 0) return rc;

  // Create device
  vblk->devno = dev_make("vd#", &virtioblk_driver, unit, vblk);
  virtio_setup_complete(&vblk->vd, 1);
  kprintf(KERN_INFO "%s: virtio disk, %dMB\n", device(vblk->devno)->name, vblk->capacity / (1024 * 1024 / SECTORSIZE));

  return 0;
}

int __declspec(dllexport) virtioblk(struct unit *unit, char *opts)
{
  return install_virtioblk(unit);
}

void init_vblk()
{
  // Try to find a virtio block device for booting
  struct unit *unit = lookup_unit_by_subunit(NULL, 0x1AF40002, 0xFFFFFFFF);
  if (unit) install_virtioblk(unit);
}
