#pragma once
#ifndef VIRTIO_QUEUE_HPP
#define VIRTIO_QUEUE_HPP

#include <modern_virtio/control_plane.hpp>
#include <info>

#include <cstddef>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>
#include <span>

using VirtBuffer = std::span<uint8_t>;

typedef struct VirtToken {
  uint16_t flags;
  VirtBuffer buffer;
    
  VirtToken(
    uint16_t flag, 
    uint8_t *buff, 
    size_t bufl
  ) : flags(flag), buffer(buff, bufl) {
    //INFO2("Constructing a VirtToken!");
  }

  /* Used simply for testing that C++ NRVO is working */
  // VirtToken(const VirtToken &token) : flags(token.flags), buffer(token.buffer.data(), token.buffer.size())
  // {
  //   // INFO2("Called copy constructor for VirtToken!");
  // }
} VirtToken;
  
using std::vector;
using VirtTokens = vector<VirtToken>;
using Descriptors = vector<uint16_t>;
  
#define VIRTIO_MSI_NO_VECTOR 0xffff
  
/* Note: The Queue Size value is a power of 2 */
#define VQUEUE_MAX_SIZE  32768
  
/* Split queue alignment requirements */
#define DESC_TBL_ALIGN   16
#define AVAIL_RING_ALIGN 2
#define USED_RING_ALIGN  4
  
/* Descriptor table stuff */
#define VIRTQ_DESC_F_NOFLAGS  0
#define VIRTQ_DESC_F_NEXT     1
#define VIRTQ_DESC_F_WRITE    2
#define VIRTQ_DESC_F_INDIRECT 4
  
typedef struct __attribute__((packed)) {
  uint64_t addr;  /* Address (guest-physical) */
  uint32_t len;   /* Length */
  uint16_t flags; /* The flags as indicated above */
  uint16_t next;  /* Next field if flags & NEXT */
} virtq_desc;
  
#define DESC_TBL_SIZE(x) (x * sizeof(virtq_desc))
  
/* Available ring stuff */
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1
#define VIRTQ_AVAIL_F_INTERRUPT    0
  
typedef struct __attribute__((packed)) {
  uint16_t flags;            /* Flags for the avail ring */
  uint16_t idx; /* Next index modulo queue size to insert */
  uint16_t ring[];           /* Ring of descriptors */
} virtq_avail;
  
#define AVAIL_RING_SIZE(x) (sizeof(virtq_avail) + x * sizeof(uint16_t))
  
/* Used ring stuff */
#define VIRTQ_USED_F_NO_NOTIFY 1
#define VIRTQ_USED_F_NOTIFY    0
  
typedef struct __attribute__((packed)) {
  uint32_t id;  /* Index of start of used descriptor chain. */
  uint32_t len; /* Bytes written into the device writable potion of the buffer chain */
} virtq_used_elem;
  
typedef struct __attribute__((packed)) {
  uint16_t flags;         /* Flags for the used ring */
  volatile uint16_t idx;  /* Flags  */
  virtq_used_elem ring[]; /* Ring of descriptors */
} virtq_used;
  
#define USED_RING_SIZE(x) (sizeof(virtq_used) + x * sizeof(virtq_used_elem))

/*
  Virtio split queue implementation
 */
class Split_queue {
public:
  Split_queue(Virtio_control& virtio_dev, int vqueue_id, 
    bool use_polling, uint8_t msix_vector = 0);
  ~Split_queue();
  
  /** Interface methods for virtqueues */
  // NOTE: The enqueued buffer pointers are assumed to be 
  // guest physical addresses
  void enqueue(VirtTokens& tokens);
  VirtTokens dequeue(uint32_t *device_written_len = nullptr);
  // NOTE: Expensive to use. An efficient driver reduce the # of kicks.
  // Enqueue multiple chains and then kick.
  void kick();
  
  uint16_t free_desc_space() const { return _free_list.size(); };
  inline uint16_t desc_space_cap() const { return _QUEUE_SIZE; }
  bool has_processed_used() const { return _last_used_idx == _used_ring->idx; };
  
  /** Methods for handling supression */
  inline void suppress() { _avail_ring->flags = VIRTQ_AVAIL_F_NO_INTERRUPT; }
  inline void unsuppress() { _avail_ring->flags = VIRTQ_AVAIL_F_INTERRUPT; }
  
protected:
  inline void _notify_device() { *_avail_notify = _VQUEUE_ID; }
  
  virtq_desc *_desc_table;
  virtq_avail *_avail_ring;
  virtq_used *_used_ring;
    
  volatile uint16_t* _avail_notify;
  uint16_t _QUEUE_SIZE;
  uint16_t _last_used_idx;
  
private:
  vector<uint16_t> _free_list;
  Virtio_control& _virtio_dev;
  int _VQUEUE_ID;
};

#endif