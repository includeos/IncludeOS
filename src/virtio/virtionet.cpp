//
// virtionet.c
//
// Network device driver for virtio
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

//#include <os/krnl.h>
#include <os>
#include <virtio/virtio.h>
#include <virtio/ether.h>
#include <virtio/pbuf.h>
#include <hw/dev.h>
#include <string.h>
#include <malloc.h>

#define MTUSIZE 1514
#define MAXSEGS 4

//
// Feature bits
//

#define VIRTIO_NET_F_CSUM       (1 << 0)       // Host handles pkts w/ partial checksum
#define VIRTIO_NET_F_GUEST_CSUM (1 << 1)       // Guest handles pkts w/ partial checksum
#define VIRTIO_NET_F_MAC        (1 << 5)       // Host has MAC address
#define VIRTIO_NET_F_GSO        (1 << 6)       // Host handles pkts with any GSO type
#define VIRTIO_NET_F_GUEST_TSO4 (1 << 7)       // Guest can handle TSOv4 in
#define VIRTIO_NET_F_GUEST_TSO6 (1 << 8)       // Guest can handle TSOv6 in
#define VIRTIO_NET_F_GUEST_ECN  (1 << 9)       // Guest can handle TSO[6] with ECN in
#define VIRTIO_NET_F_GUEST_UFO  (1 << 10)      // Guest can handle UFO in
#define VIRTIO_NET_F_HOST_TSO4  (1 << 11)      // Host can handle TSOv4 in
#define VIRTIO_NET_F_HOST_TSO6  (1 << 12)      // Host can handle TSOv6 in
#define VIRTIO_NET_F_HOST_ECN   (1 << 13)      // Host can handle TSO[6] w/ ECN in
#define VIRTIO_NET_F_HOST_UFO   (1 << 14)      // Host can handle UFO in
#define VIRTIO_NET_F_MRG_RXBUF  (1 << 15)      // Host can merge receive buffers
#define VIRTIO_NET_F_STATUS     (1 << 16)      // virtio_net_config status available
#define VIRTIO_NET_F_CTRL_VQ    (1 << 17)      // Control channel available
#define VIRTIO_NET_F_CTRL_RX    (1 << 18)      // Control channel RX mode support
#define VIRTIO_NET_F_CTRL_VLAN  (1 << 19)      // Control channel VLAN filtering

//
// Virtual NIC configuration block
//

#define VIRTIO_NET_S_LINK_UP    1              // Link is up

struct virtio_net_config 
{
  struct eth_addr mac;
  unsigned short status;
};

//
// Virtual NIC device packet header
//

#define VIRTIO_NET_HDR_F_NEEDS_CSUM     1       // Use csum_start, csum_offset
#define VIRTIO_NET_HDR_F_DATA_VALID     2       // Csum is valid

#define VIRTIO_NET_HDR_GSO_NONE         0       // Not a GSO frame
#define VIRTIO_NET_HDR_GSO_TCPV4        1       // GSO frame, IPv4 TCP (TSO)
#define VIRTIO_NET_HDR_GSO_UDP          3       // GSO frame, IPv4 UDP (UFO)
#define VIRTIO_NET_HDR_GSO_TCPV6        4       // GSO frame, IPv6 TCP
#define VIRTIO_NET_HDR_GSO_ECN          0x80    // TCP has ECN set

struct virtio_net_hdr
{
  unsigned char flags;
  unsigned char gso_type;
  unsigned short hdr_len;          // Ethernet + IP + TCP/UDP headers
  unsigned short gso_size;         // Bytes to append to hdr_len per frame
  unsigned short csum_start;       // Position to start checksumming from
  unsigned short csum_offset;      // Offset after that to place checksum
};

//
// Virtual network device data
//

struct virtionet
{
  struct virtio_device vd;
  struct virtio_net_config config;
  struct virtio_queue rxqueue;
  struct virtio_queue txqueue;
  dev_t devno;
};


static int add_receive_buffer(struct virtionet *vnet)
{
  struct virtio_net_hdr *hdr;
  struct scatterlist sg[2];

  struct pbuf *p = pbuf_alloc(PBUF_RAW, MTUSIZE + sizeof(struct virtio_net_hdr), PBUF_RW);
  if (!p) return -ENOMEM;
  hdr = (virtio_net_hdr*)p->payload;
  pbuf_header(p, -(int) sizeof(struct virtio_net_hdr));
  sg[0].data = hdr;
  sg[0].size = sizeof(struct virtio_net_hdr);
  sg[1].data = p->payload;
  sg[1].size = p->len;
  virtio_enqueue(&vnet->rxqueue, sg, 0, 2, p);

  return 0;
}

static int virtionet_ioctl(struct dev *dev, int cmd, void *args, size_t size)
{
  return -ENOSYS;
}

static int virtionet_rx_callback(struct virtio_queue *vq)
{
  struct virtionet *vnet = (struct virtionet *) vq->vd;
  struct pbuf *p;
  unsigned int len;
  int rc, received;

  // Drain receive queue
  received = 0;
  while ((p = (pbuf*)virtio_dequeue(vq, &len)) != NULL)
  {
    pbuf_realloc(p, len);
    rc = dev_receive(vnet->devno, p);
    if (rc < 0) pbuf_free(p);
    received++;
  }

  // Fill up receive queue with new empty buffers
  if (received > 0)
  {
    while (received > 0) {
      add_receive_buffer(vnet);
      received--;
    }
    virtio_kick(&vnet->rxqueue);
  }

  return 0;
}
*/
static int virtionet_tx_callback(struct virtio_queue *vq)
{
  struct pbuf *hdr;
  struct pbuf *data;
  unsigned int len;

  // Deallocate packets buffers after they have been transmitted.
  while ((hdr = (pbuf*)virtio_dequeue(vq, &len)) != NULL)
  {
    data = pbuf_dechain(hdr);
    pbuf_free(hdr);
    pbuf_free(data);
  }

  return 0;
}


int virtionet_attach(struct dev *dev, struct eth_addr *hwaddr)
{
  struct virtionet *vnet = (virtionet*)dev->privdata;
  *hwaddr = vnet->config.mac;

  return 0;
}

int virtionet_detach(struct dev *dev)
{
  return 0;
}


int virtionet_transmit(struct dev *dev, struct pbuf *p)
{
  struct virtionet *vnet = (virtionet*)dev->privdata;
  struct pbuf *hdr;
  struct pbuf *q;
  int i;
  struct scatterlist sg[MAXSEGS];
  
  // Allocate packet header
  hdr = pbuf_alloc(PBUF_RAW, sizeof(struct virtio_net_hdr), PBUF_RW);
  if (hdr == NULL) return -ENOMEM;
  memset(hdr->payload, 0, sizeof(struct virtio_net_hdr));
  pbuf_chain(hdr, p);

  // Add packet to transmit queue
  for (i = 0, q = hdr; q; q = q->next, i++)
  {
    if (i == MAXSEGS) -ERANGE;
    sg[i].data = q->payload;
    sg[i].size = q->len;
  }
  virtio_enqueue(&vnet->txqueue, sg, 2, 0, p);
  virtio_kick(&vnet->txqueue);

  return 0;  
}

struct driver virtionet_driver =
{
  "virtionet",
  DEV_TYPE_PACKET,
  virtionet_ioctl,
  NULL,
  NULL,
  virtionet_attach,
  virtionet_detach,
  virtionet_transmit
};

*/
extern "C" {
int virtio_install(struct unit *unit, char *opts)
{
  struct virtionet *vnet;
  int rc, size, i;

  // Setup unit information
  if (!unit) return -ENOSYS;
  unit->vendorname = "VIRTIO";
  unit->productname = "VIRTIO Virtual Network Device";
  
  // Allocate memory for device
  vnet = (virtionet*)kmalloc(sizeof(struct virtionet));
  if (vnet == NULL) return -ENOMEM;
  memset(vnet, 0, sizeof(struct virtionet));

  // Initialize virtual device
  rc = virtio_device_init(&vnet->vd, unit, 0);
  if (rc < 0) return rc;
  
  // Get block device configuration
  virtio_get_config(&vnet->vd, &vnet->config, sizeof(vnet->config));
  
  // Initialize transmit and receive queues  
  /*rc = virtio_queue_init(&vnet->rxqueue, &vnet->vd, 0, virtionet_rx_callback);
  if (rc < 0) return rc;
  rc = virtio_queue_init(&vnet->txqueue, &vnet->vd, 1, virtionet_tx_callback);
  if (rc < 0) return rc;

  // Fill receive queue
  size = virtio_queue_size(&vnet->rxqueue) / 2;
  for (i = 0; i < size; ++i) add_receive_buffer(vnet);

  */
  virtio_kick(&vnet->rxqueue);

  // Create device
  /*o
  vnet->devno = dev_make("eth#", &virtionet_driver, unit, vnet);
  */
  virtio_setup_complete(&vnet->vd, 1);
  kprintf(KERN_INFO "%s: virtio net, mac %la\n", device(vnet->devno)->name, &vnet->config.mac);

  return 0;
}

}
