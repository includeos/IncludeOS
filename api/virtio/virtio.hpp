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

/**
   @note This virtio implementation was very much inspired by
   SanOS, (C) Michael Ringgaard. All due respect.

   STANDARD:

   We're aiming to become standards compilant according to this one:

   Virtio 1.0, OASIS Committee Specification Draft 03
   (http://docs.oasis-open.org/virtio/virtio/v1.0/virtio-v1.0.html)

   In the following abbreviated to Virtio 1.03 or Virtio std.
*/
#pragma once
#ifndef VIRTIO_VIRTIO_HPP
#define VIRTIO_VIRTIO_HPP

#include "../hw/pci_device.hpp"
#include <net/inet_common.hpp>
#include <stdint.h>
#include <vector>
#include <common>

#define PAGE_SIZE 4096

#define VIRTIO_F_NOTIFY_ON_EMPTY 24
#define VIRTIO_F_ANY_LAYOUT 27
#define VIRTIO_F_RING_INDIRECT_DESC 28
#define VIRTIO_F_RING_EVENT_IDX 29
#define VIRTIO_F_VERSION_1 32

// From sanos virtio.h
#define VIRTIO_PCI_HOST_FEATURES        0   // Features supported by the host
#define VIRTIO_PCI_GUEST_FEATURES       4   // Features activated by the guest
#define VIRTIO_PCI_QUEUE_PFN            8   // PFN for the currently selected queue
#define VIRTIO_PCI_QUEUE_SIZE           12  // Queue size for the currently selected queue
#define VIRTIO_PCI_QUEUE_SEL            14  // Queue selector
#define VIRTIO_PCI_QUEUE_NOTIFY         16  // Queue notifier
#define VIRTIO_PCI_STATUS               18  // Device status register
#define VIRTIO_PCI_ISR                  19  // Interrupt status register
#define VIRTIO_PCI_CONFIG               20  // Configuration data offset
#define VIRTIO_PCI_CONFIG_MSIX          24  // .. when MSI-X is enabled


#define VIRTIO_CONFIG_S_ACKNOWLEDGE     1
#define VIRTIO_CONFIG_S_DRIVER          2
#define VIRTIO_CONFIG_S_DRIVER_OK       4
#define VIRTIO_CONFIG_S_FAILED          0x80


//#include <class_irq_handler.hpp>
class Virtio
{

public:
  /** A wrapper for buffers to be passed in to the Queue */
  class Token {

  public:
    // "Direction" of tokens
    using span = std::pair<uint8_t*, size_t>;  //gsl::span<uint8_t>;
    using size_type = size_t;//span::size_type;
    enum Direction { IN, OUT };
    inline Token(span buf, Direction d) :
      data_{ buf.first }, size_{ buf.second }, dir_{ d }
    {}

    inline auto data() { return data_; }
    inline auto size() { return size_; }
    inline auto direction() { return dir_; }


  private:
    uint8_t* data_;
    size_type size_;
    Direction dir_;
  };

  // http://docs.oasis-open.org/virtio/virtio/v1.0/csprd01/virtio-v1.0-csprd01.html#x1-860005
  // Virtio device types
  enum virtiotype_t
    {
      RESERVED = 0,
      NIC,
      BLOCK,
      CONSOLE,
      ENTROPY,
      BALLOON,
      IO_MEM,
      RP_MSG,
      SCSI_HOST,
      T9P,
      WLAN,
      RP_SERIAL,
      CAIF
    };

  /** Virtio Queue class. */
  class Queue {
  public:
    /** @note Using typedefs in order to keep the standard notation. */
    using le64 =  uint64_t;
    using le32 = uint32_t;
    using le16 = uint16_t;
    using u16 = uint16_t;
    using u8 = uint8_t;

    /** Virtio Ring Descriptor. Virtio std. §2.4.5  */
    struct virtq_desc {
      /* Address (guest-physical). */
      le64 addr;
      /* Length. */
      le32 len;

      /* This marks a buffer as continuing via the next field. */
#define VIRTQ_DESC_F_NEXT     1
      /* This marks a buffer as device write-only (otherwise device read-only). */
#define VIRTQ_DESC_F_WRITE    2
      /* This means the buffer contains a list of buffer descriptors. */
#define VIRTQ_DESC_F_INDIRECT 4
      /* The flags as indicated above. */
      le16 flags;
      /* Next field if flags & NEXT */
      le16 next;
    };


    /** Virtio Available ring. Virtio std. §2.4.6 */
    struct virtq_avail {
#define VIRTQ_AVAIL_F_NO_INTERRUPT      1
      le16 flags;
      le16 idx;
      le16 ring[/* Queue Size */];
      /*le16 used_event;  Only if VIRTIO_F_EVENT_IDX */
    };


    /** Virtio Used ring elements. Virtio std. §2.4.8 */
    struct virtq_used_elem {
      /* le32 is used here for ids for padding reasons. */
      /* Index of start of used descriptor chain. */
      le32 id;
      /* Total length of the descriptor chain which was used (written to) */
      le32 len;
    };

    /** Virtio Used ring. Virtio std. §2.4.8 */
    struct virtq_used {
#define VIRTQ_USED_F_NO_NOTIFY  1
      le16 flags;
      le16 idx;
      struct virtq_used_elem ring[ /* Queue Size */];
      /*le16 avail_event; Only if VIRTIO_F_EVENT_IDX */
    };


    /** Virtqueue. Virtio std. §2.4.2 */
    struct virtq {
      // The actual descriptors (i.e. tokens) (16 bytes each)
      virtq_desc* desc;// [ /* Queue Size*/  ];

      // A ring of available descriptor heads with free-running index.
      virtq_avail* avail;

      // Padding to the next PAGE_SIZE boundary.
      u8* pad; //[ /* Padding */ ];

      // A ring of used descriptor heads with free-running index.
      virtq_used* used;
    };

    /** Virtque size calculation. Virtio std. §2.4.2 */
    static inline unsigned virtq_size(unsigned int qsz);

    /**
       Update the available index */
    inline void update_avail_idx ()
    {
#if defined(ARCH_x86)
      // Std. §3.2.1 pt. 4
      __arch_hw_barrier();
      _queue.avail->idx += _num_added;
      _num_added = 0;
#else
#warning "update_avail_idx() not implemented for selected arch"
#endif
    }

    /** Kick hypervisor.

        Will notify the host (Qemu/Virtualbox etc.) about pending data  */
    void kick();

    /** Constructor. @param size shuld be fetched from PCI device. */
    Queue() {}
    Queue(const std::string& name,
          uint16_t size, uint16_t q_index, uint16_t iobase);

    /** Get the queue descriptor. To be written to the Virtio device. */
    virtq_desc* queue_desc() const { return _queue.desc; }

    /** Push data tokens onto the queue.
        @param buffers : A span of tokens
    */
    int enqueue(gsl::span<Virtio::Token> buffers);

    /** Dequeue a received packet */
    Token dequeue();

    void disable_interrupts();
    void enable_interrupts();
    bool interrupts_enabled() const noexcept;

    /** Release token. @param head : the token ID to release*/
    void release(uint32_t head);

    /** Get number of new incoming buffers, i.e. the increase in
        queue_used_->idx since we last checked. An increase means the device
        has inserted tokens into the used ring.*/
    uint16_t new_incoming() const noexcept
    { return _queue.used->idx - _last_used_idx; }

    /** Get number of used buffers */
    uint16_t num_used() const noexcept
    { return _queue.avail->idx - _queue.used->idx; }

    uint16_t num_inflight() const noexcept
    {
      return _desc_in_flight;
    }

    /** Get number of free tokens in Queue */
    uint16_t num_free() const noexcept
    {
      //Expects(size() - _free_head == size() - _desc_in_flight);
      return size() - _desc_in_flight;
    }

    // access the current index
    virtq_desc& current()
    {
      return _queue.desc[_free_head];
    }
    virtq_desc& next()
    {
      return _queue.desc[ _queue.desc[_free_head].next ];
    }

    // go to next index
    void go_next()
    {
      _free_head = _queue.desc[_free_head].next;
    }

    uint16_t size() const noexcept
    {
      return _size;
    }

  private:
    /** Initialize the queue buffer */
    void init_queue(int size, char* buf);

    std::string qname;

    // The size as read from the PCI device
    uint16_t _size;

    // The actual queue struct
    virtq _queue;

    uint16_t _iobase = 0; // Device PCI location
    uint16_t _free_head = 0; // First available descriptor (_queue.desc[_free_head])
    uint16_t _num_added = 0; // Entries to be added to _queue.avail->idx
    uint16_t _desc_in_flight = 0; // Entries in _queue_desc currently in use
    uint16_t _last_used_idx = 0; // Last known value of _queue.used->idx
    uint16_t _pci_index = 0; // Queue nr.
  };


  /** Get the Virtio config registers from the PCI device.

      @note it varies how these are structured, hence a void* buf */
  void get_config(void* buf, int len);

  /** Get the list of subscribed IRQs */
  auto& get_irqs() { return irqs; };

  /** Get the legacy PCI IRQ */
  uint8_t get_legacy_irq();

  /** Reset the virtio device */
  void reset();

  /** Negotiate supported features with host */
  void negotiate_features(uint32_t features);

  /** Probe PCI device for features */
  uint32_t probe_features();

  /** Get locally stored features */
  inline uint32_t features(){ return _features; };

  /** Get iobase. Wrapper around PCI_Device::iobase */
  inline uint32_t iobase(){ return _iobase; }

  /** Get queue size. @param index - the Virtio queue index */
  uint32_t queue_size(uint16_t index);

  /** Assign a queue descriptor to a PCI queue index */
  bool assign_queue(uint16_t index, const void* queue_desc);

  /** Tell Virtio device if we're OK or not. Virtio Std. § 3.1.1,step 8*/
  void setup_complete(bool ok);

  /** Indicate which Virtio version (PCI revision ID) is supported.

      Currently only Legacy is supported (partially the 1.0 standard)
  */
  static inline bool version_supported(uint16_t i) { return i <= 0; }

  // returns true if MSI-X is supported
  bool has_msix() const noexcept {
    return _pcidev.has_msix();
  }
  // returns non-zero if MSI-x is supported
  uint8_t get_msix_vectors() const noexcept {
    return _pcidev.get_msix_vectors();
  }

  void move_to_this_cpu();

  /** Virtio device constructor.

      Should conform to Virtio std. §3.1.1, steps 1-6
      (Step 7 is "Device specific" which a subclass will handle)
  */
  Virtio(hw::PCI_Device& pci);

protected:
  void deactivate_msix() {
    _pcidev.deactivate_msix();
  }
private:
  hw::PCI_Device& _pcidev;

  //We'll get this from PCI_device::iobase(), but that lookup takes longer
  uint32_t _iobase = 0;
  uint32_t _features = 0;
  uint16_t _virtio_device_id = 0;

  // Indicate if virtio device ID is legacy or standard
  bool _LEGACY_ID = 0;
  bool _STD_ID = 0;

  void default_irq_handler();

  uint8_t current_cpu;
  std::vector<uint8_t> irqs;
};

#endif
