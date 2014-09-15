//
// ether.h
//
// Ethernet network
//
// Copyright (C) 2002 Michael Ringgaard. All rights reserved.
// Portions Copyright (C) 2001, Swedish Institute of Computer Science.
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

#ifndef ETHER_H
#define ETHER_H



#pragma pack(push, 1)

#define ETHER_HLEN 14

#define ETHER_ADDR_LEN 6

struct eth_addr 
{
  unsigned char addr[ETHER_ADDR_LEN];
};
  
struct eth_hdr 
{
  struct eth_addr dest;
  struct eth_addr src;
  unsigned short type;
};

#pragma pack(pop)

__inline int eth_addr_isbroadcast(struct eth_addr *addr)
{
  int n;
  for (n = 0; n < ETHER_ADDR_LEN; n++) if (addr->addr[n] != 0xFF) return 0;
  return 1;
}

krnlapi char *ether2str(struct eth_addr *hwaddr, char *s);
krnlapi unsigned long ether_crc(int length, unsigned char *data);

krnlapi struct netif *ether_netif_add(char *name, char *devname, struct ip_addr *ipaddr, struct ip_addr *netmask, struct ip_addr *gw);

krnlapi err_t ether_input(struct netif *netif, struct pbuf *p);
krnlapi err_t ether_output(struct netif *netif, struct pbuf *p, struct ip_addr *ipaddr);

void ether_init();
int register_ether_netifs();

#endif
