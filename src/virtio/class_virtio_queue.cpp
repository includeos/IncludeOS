#include <virtio/class_virtio.hpp>
#include <malloc.h>
#include <string.h>
#include <syscalls.hpp>
#include <virtio/virtio.h>
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
  printf("\t * Queue desc  @ 0x%lx \n ",(long)_queue.desc);

  // The available buffer starts right after the queue descriptors
  _queue.avail = (virtq_avail*)((char*)buf + size*sizeof(virtq_desc));  
  printf("\t * Queue avail @ 0x%lx \n ",(long)_queue.avail);

  // The used queue starts at the beginning of the next page
  // (This is  a formula from sanos - don't know why it works, but it does
  // align the used queue to the next page border)  
  _queue.used = (virtq_used*)(((uint32_t)&_queue.avail->ring[size] +
                                sizeof(uint16_t)+PAGESIZE-1) & ~(PAGESIZE -1));
  printf("\t * Queue used  @ 0x%lx \n ",(long)_queue.used);
  
}

/** Constructor */
Virtio::Queue::Queue(uint16_t size, uint16_t q_index, uint16_t iobase)
  : _size(size),_size_bytes(virtq_size(size)),_iobase(iobase),_num_free(size),
    _free_head(0), _num_added(0),_last_used_idx(0),_pci_index(q_index)
{
  //Allocate space for the queue and clear it out
  void* buffer = memalign(PAGE_SIZE,_size_bytes);
  //void* buffer = malloc(_size_bytes);
  if (!buffer) panic("Could not allocate space for Virtio::Queue");
  memset(buffer,0,_size_bytes);    

  printf(">>> Virtio Queue of size %i (%li bytes) initializing \n",
         _size,_size_bytes);  
  init_queue(size,buffer);
  
  printf("\t * Chaining buffers \n");  
   // Chain buffers
  //for (int i=0; i<size; i++) _queue.desc[i].next = i +1;
  
  // Allocate space for actual data tokens
  //_data = (void**) malloc(sizeof(void*) * size);
  
    
  printf(" >> Virtio Queue setup complete. \n");
}


/** Ported more or less directly from SanOS. */
int Virtio::Queue::enqueue(scatterlist sg[], uint32_t out, uint32_t in, void* UNUSED(data)){
  
  int i,avail,head, prev = _free_head;
  

  while (_num_free < out + in) //@todo wait... or something.
    panic("Trying to enqueue, but NO FREE BUFFERS");
    // Remove buffers from the free list
  
  _num_free -= out + in;
  head = _free_head;
  
  
  // (implicitly) Mark all outbound tokens as device-readable
  for (i = _free_head; out; i = _queue.desc[i].next, out--) 
  {
    _queue.desc[i].flags = VRING_DESC_F_NEXT;
    _queue.desc[i].addr = (uint64_t)sg->data;
    _queue.desc[i].len = sg->size;
    prev = i;
    sg++;
  }
  
  // Mark all inbound tokens as device-writable
  for (; in; i = _queue.desc[i].next, in--) 
  {
    _queue.desc[i].flags = VRING_DESC_F_NEXT | VRING_DESC_F_WRITE;
    _queue.desc[i].addr = (uint64_t)sg->data;
    _queue.desc[i].len = sg->size;
    prev = i;
    sg++;
  }
  
  // No continue on last buffer
  _queue.desc[prev].flags &= ~VRING_DESC_F_NEXT;

  // Update free pointer
  _free_head = i;

  // Set callback token
  //vq->data[head] = data;

  // SanOS: Put entry in available array, but do not update avail->idx until sync
  avail = (_queue.avail->idx + _num_added++) % _size;
  _queue.avail->ring[avail] = head;
  
  
  // Notify about free buffers
  //if (_num_free > 0) set_event(&vq->bufavail);
    
  return _num_free;  
}

void Virtio::Queue::release(uint32_t head){
  
  // Clear callback data token
  //vq->data[head] = NULL;

  // Put buffers back on the free list; first find the end
  uint32_t i = head;
  while (_queue.desc[i].flags & VRING_DESC_F_NEXT) 
  {
    i = _queue.desc[i].next;
    _num_free++;
  }
  _num_free++;

  // Add buffers back to free list
  _queue.desc[i].next = _free_head;
  _free_head = head;

  // Notify about free buffers
  //if (_num_free > 0) set_event(&vq->bufavail);
}

void* Virtio::Queue::dequeue(uint32_t* len){
  struct virtq_used_elem *e;
  void *data;

  // Return NULL if there are no more completed buffers in the queue
  if (! _last_used_idx != _queue.used->idx){
    printf("...Supposedly there are no used buffers \n");
    return NULL;
  }

  // Get next completed buffer
  e = &_queue.used->ring[_last_used_idx % _size];
  *len = e->len;
  
  //data = vq->data[e->id];
  
  // Release buffer
  release(e->id);
  _last_used_idx++;

  return data;
}

//TEMP. REMOVE - This belongs to VirtioNet.
struct virtio_net_hdr
  {
    uint8_t flags;
    uint8_t gso_type;
    uint16_t hdr_len;          // Ethernet + IP + TCP/UDP headers
    uint16_t gso_size;         // Bytes to append to hdr_len per frame
    uint16_t csum_start;       // Position to start checksumming from
    uint16_t csum_offset;      // Offset after that to place checksum
    uint16_t num_buffers;      // ONLY if "Merge RX-buffers"
  };


// TEMP. REMOVE - This belongs to ... well, ethernet.
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

char *ether2str(struct eth_addr *hwaddr, char *s) {
  sprintf(s, "%02x:%02x:%02x:%02x:%02x:%02x",  
          hwaddr->addr[0], hwaddr->addr[1], hwaddr->addr[2], 
          hwaddr->addr[3], hwaddr->addr[4], hwaddr->addr[5]);
  return s;
}

void Virtio::Queue::notify(){
  printf("\t <VirtQueue> Notified, checking buffers.... \n");
  printf("\t             Used idx: %i, Avail idx: %i \n",
           _queue.used->idx, _queue.avail->idx );
  
  int new_packets = _queue.used->idx - _last_used_idx;
  if (!new_packets) return;
  
  printf("\t <VirtQueue> %i new packets: \n", new_packets);
    
  // For each token
  for (;_last_used_idx != _queue.used->idx; _last_used_idx++){
    auto id = _queue.used->ring[_last_used_idx % _size].id;
    auto len = _queue.used->ring[_last_used_idx % _size].len;
    printf("\t             Packet id: 0x%lx len: 0x%lx \n",id,len);
    
    
      // The first token should be a virtio header
    auto chunksize =  _queue.desc[id].len;
    assert(chunksize == sizeof(virtio_net_hdr));
    
    //auto addr = _queue.desc[id].addr;      
    /** Extract the Virtio header     
        virtio_net_hdr* hdr = (virtio_net_hdr*)addr;
        // Print it 
        printf("VirtioNet Header: \n"           \
        "Flags: 0x%x \n"                        \
             "GSO Type: 0x%x \n"                \
             "Header length: %i \n"             \
             "GSO Size: %i \n"                  \
             "Csum.start: %i \n"                \
             "Csum. offset: %i \n"              \
             "Num. Buffers: %i \n",
             hdr->flags,hdr->gso_type,hdr->hdr_len,
               hdr->gso_size,hdr->csum_start,hdr->csum_offset,hdr->num_buffers);
    */
    
    // This can't only be a header
    assert(_queue.desc[id].flags & VRING_DESC_F_NEXT);
    
    // Extract the next token which should be the ethernet frame
    auto next =  _queue.desc[id].next;
    auto next_addr = _queue.desc[next].addr;
    
    handle((void*)next_addr,_queue.desc[next].len);
    
    /*
    eth_hdr* eth = (eth_hdr*) next_addr;
    
    char addr[] = "00:00:00:00:00:00";
    printf("\t             Eth. Source: %s \n",ether2str(&eth->src,addr));
    printf("\t             Eth. Dest. : %s \n",ether2str(&eth->dest,addr));
    printf("\t             Eth. Type  : 0x%x\n",eth->type); 
    */
    
  /** DEBUG: These are the Device's available packages 
      printf("\t Avail packet 0: %s \n",
         (char*)_queue.desc[_queue.avail->idx].addr);
         printf("\t Avail packet 1: %s \n",
  (char*)_queue.desc[_queue.desc[_queue.avail->idx].next].addr);*/
  }
}


void Virtio::Queue::kick(){
  _queue.avail->idx += _num_added;
  _num_added = 0;
  
  if (!(_queue.used->flags & VRING_USED_F_NO_NOTIFY)){
    printf("Kicking virtio. Iobase 0x%x, Queue index %i \n",
           _iobase,_pci_index);
    outpw(_iobase + VIRTIO_PCI_QUEUE_NOTIFY , _pci_index);
  }
}
