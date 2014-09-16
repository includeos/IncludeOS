//
// pbuf.h
//
// Packet buffer management
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

#ifndef PBUF_H
#define PBUF_H

#define PBUF_TRANSPORT_HLEN 20
#define PBUF_IP_HLEN        20
#define PBUF_LINK_HLEN      14

#define PBUF_TRANSPORT      0
#define PBUF_IP             1
#define PBUF_LINK           2
#define PBUF_RAW            3

#define PBUF_RW             0
#define PBUF_RO             1
#define PBUF_POOL           2

// Definitions for the pbuf flag field (these are not the flags that
// are passed to pbuf_alloc()).

#define PBUF_FLAG_RW    0x00    // Flags that pbuf data is read/write.
#define PBUF_FLAG_RO    0x01    // Flags that pbuf data is read-only.
#define PBUF_FLAG_POOL  0x02    // Flags that the pbuf comes from the pbuf pool.

struct pbuf 
{
  struct pbuf *next;
  
  unsigned short flags;
  unsigned short ref;
  void *payload;
  
  int tot_len;                // Total length of buffer + additionally chained buffers.
  int len;                    // Length of this buffer.
  int size;                   // Allocated size of buffer
};

void pbuf_init();

krnlapi struct pbuf *pbuf_alloc(int layer, int size, int type);
krnlapi void pbuf_realloc(struct pbuf *p, int size); 
krnlapi int pbuf_header(struct pbuf *p, int header_size);
krnlapi int pbuf_clen(struct pbuf *p);
krnlapi int pbuf_spare(struct pbuf *p);
krnlapi void pbuf_ref(struct pbuf *p);
krnlapi int pbuf_free(struct pbuf *p);
krnlapi void pbuf_chain(struct pbuf *h, struct pbuf *t);
krnlapi struct pbuf *pbuf_dechain(struct pbuf *p);
krnlapi struct pbuf *pbuf_dup(int layer, struct pbuf *p);
krnlapi struct pbuf *pbuf_linearize(int layer, struct pbuf *p);
krnlapi struct pbuf *pbuf_cow(int layer, struct pbuf *p);

#endif
