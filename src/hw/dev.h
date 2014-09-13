//
// dev.h
//
// Device Manager
//
// Copyright (C) 2002 Michael Ringgaard. All rights reserved.
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

#ifndef DEV_H
#define DEV_H

#include "sanos_port.h"

struct devfile;

struct dev;
struct bus;
struct unit;

#define NODEV (-1)

#define DEVNAMELEN              32
#define MAX_DEVS                64

#define SECTORSIZE              512

#define DEV_TYPE_STREAM         1
#define DEV_TYPE_BLOCK          2
#define DEV_TYPE_PACKET         3

#define DEVFLAG_NBIO            1

#define IOCTL_GETBLKSIZE        1
#define IOCTL_GETDEVSIZE        2
#define IOCTL_GETGEOMETRY       3
#define IOCTL_REVALIDATE        4

#define RESOURCE_IO             1
#define RESOURCE_MEM            2
#define RESOURCE_IRQ            3
#define RESOURCE_DMA            4

#define BUSTYPE_HOST            0
#define BUSTYPE_PCI             1
#define BUSTYPE_ISA             2

#define BIND_BY_CLASSCODE       1
#define BIND_BY_UNITCODE        2
#define BIND_BY_SUBUNITCODE     3

//
// Bus
//

struct bus
{
  struct bus *next;
  struct bus *sibling;
  struct bus *parent;

  struct unit *self;

  unsigned long bustype;
  unsigned long busno;

  struct unit *units;
  struct bus *bridges;
};

//
// Unit
//

struct unit
{
  struct unit *next;
  struct unit *sibling;
  struct bus *bus;
  struct dev *dev;

  unsigned long classcode;
  unsigned long unitcode;
  unsigned long subunitcode;
  unsigned long revision;
  unsigned long unitno;

  const char *classname;
  const char *vendorname;
  const char *productname;
    
  struct resource *resources;
};

//
// Resource
//

struct resource
{
  struct resource *next;
  unsigned short type;
  unsigned short flags;
  unsigned long start;
  unsigned long len;
};

//
// Binding
//

struct binding
{
  int bindtype;
  unsigned long bustype;
  unsigned long code;
  unsigned long mask;
  char *module;
};

//
// Driver
//

struct driver
{
  char *name;
  int type;

  int (*ioctl)(struct dev *dev, int cmd, void *args, size_t size);
  int (*read)(struct dev *dev, void *buffer, size_t count, blkno_t blkno, int flags);
  int (*write)(struct dev *dev, void *buffer, size_t count, blkno_t blkno, int flags);

  int (*attach)(struct dev *dev, struct eth_addr *hwaddr);
  int (*detach)(struct dev *dev);
  int (*transmit)(struct dev *dev, struct pbuf *p);
  int (*set_rx_mode)(struct dev *dev);
};

//
// Device
//

struct dev 
{
  char name[DEVNAMELEN];
  struct driver *driver;
  struct unit *unit;
  void *privdata;
  int refcnt;
  uid_t uid;
  gid_t gid;
  int mode;
  struct devfile *files;
  
  int reads;
  int writes;
  int input;
  int output;

  struct netif *netif;
  int (*receive)(struct netif *netif, struct pbuf *p);
};

//
// Geometry
//

struct geometry
{
  int cyls;
  int heads;
  int spt;
  int sectorsize;
  int sectors;
};

//
// Board info
//

struct board
{
  char *vendorname;
  char *productname;
  unsigned long bustype;
  unsigned long unitcode;
  unsigned long unitmask;
  unsigned long subsystemcode;
  unsigned long subsystemmask;
  unsigned long revisioncode;
  unsigned long revisionmask;
  int flags;
};

extern struct dev *devtab[];
extern unsigned int num_devs;

extern struct unit *units;
extern struct bus *buses;

void enum_host_bus();
void install_drivers();

 struct bus *add_bus(struct unit *self, unsigned long bustype, unsigned long busno);
 struct unit *add_unit(struct bus *bus, unsigned long classcode, unsigned long unitcode, unsigned long unitno);
 struct resource *add_resource(struct unit *unit, unsigned short type, unsigned short flags, unsigned long start, unsigned long len);

 struct resource *get_unit_resource(struct unit *unit, int type, int num);
 int get_unit_irq(struct unit *unit);
 int get_unit_iobase(struct unit *unit);
 void *get_unit_membase(struct unit *unit);
 char *get_unit_name(struct unit *unit);

 struct unit *lookup_unit(struct unit *start, unsigned long unitcode, unsigned long unitmask);
 struct unit *lookup_unit_by_subunit(struct unit *start, unsigned long subunitcode, unsigned long subunitmask);
 struct unit *lookup_unit_by_class(struct unit *start, unsigned long classcode, unsigned long classmask);

 struct board *lookup_board(struct board *board_tbl, struct unit *unit);

 struct dev *device(dev_t devno);

 dev_t dev_make(char *name, struct driver *driver, struct unit *unit, void *privdata);
 dev_t devno(char *name);
 dev_t dev_open(char *name);
 int dev_close(dev_t devno);

 int dev_ioctl(dev_t devno, int cmd, void *args, size_t size);
 int dev_read(dev_t devno, void *buffer, size_t count, blkno_t blkno, int flags);
 int dev_write(dev_t devno, void *buffer, size_t count, blkno_t blkno, int flags);

 int dev_attach(dev_t dev, struct netif *netif, int (*receive)(struct netif *netif, struct pbuf *p));
 int dev_detach(dev_t devno);
 int dev_transmit(dev_t devno, struct pbuf *p);
 int dev_receive(dev_t devno, struct pbuf *p);

 int dev_setevt(dev_t devno, int events);
 int dev_clrevt(dev_t devno, int events);

#endif
