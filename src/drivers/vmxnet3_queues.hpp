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

#pragma once
#include <cstdio>

/** Maximum number of interrupts */
#define VMXNET3_MAX_INTRS 25
/** Adaptive Interrupt Moderation */
#define UPT1_IML_ADAPTIVE 0x8
/** VLAN tag stripping feature */
#define UPT1_F_RXVLAN     0x4

#define GOS_TYPE_LINUX    1
#define GOS_BITS_32_BITS  1
#define GOS_BITS_64_BITS  2

struct guestOS_bits {
  uint32_t  arch    :  2;     /* 32 or 64-bit */
  uint32_t  type    :  4;     /* which guest */
  uint32_t  version : 16;     /* os version */
  uint32_t  misc    : 10;
};

/** Miscellaneous configuration descriptor */
struct vmxnet3_misc_config {
  uint32_t version;         /** Driver version */
  guestOS_bits guest_info;  /** Guest information */
  uint32_t version_support; /** Version supported */
  uint32_t upt_version_support;  /** UPT version supported */
  uint64_t upt_features;    /** UPT features supported */
  uint64_t driver_data_address;  /** Driver-private data address */
  uint64_t queue_desc_address;   /** Queue descriptors data address */
  uint32_t driver_data_len; /** Driver-private data length */
  uint32_t queue_desc_len;  /** Queue descriptors data length */
  uint32_t mtu;             /** Maximum transmission unit */
  uint16_t max_num_rx_sg;   /** Maximum number of RX scatter-gather */
  uint8_t num_tx_queues;    /** Number of TX queues */
  uint8_t num_rx_queues;    /** Number of RX queues */
  uint32_t reserved0[4];    /** Padding */
} __attribute__ ((packed));

/** Driver version magic */
#define VMXNET3_VERSION_MAGIC 0x69505845

/** Interrupt configuration */
struct vmxnet3_interrupt_config {
  uint8_t mask_mode;
  uint8_t num_intrs;
  uint8_t event_intr_index;
  uint8_t moderation_level[VMXNET3_MAX_INTRS];
  uint32_t control;
  uint32_t reserved0[2];
} __attribute__ (( packed ));

/** Receive filter configuration */
struct vmxnet3_rx_filter_config {
  /** Receive filter mode */
  uint32_t mode;
  /** Multicast filter table length */
  uint16_t multicast_len;
  /** Reserved */
  uint16_t reserved0;
  /** Multicast filter table address */
  uint64_t multicast_address;
  /** VLAN filter table (one bit per possible VLAN) */
  uint8_t vlan_filter[512];
} __attribute__ (( packed ));

/** Rx filter configuration */
enum vmxnet3_rx_filter_mode {
  VMXNET3_RXM_UCAST = 0x01,  /**< Unicast only */
  VMXNET3_RXM_MCAST = 0x02,  /**< Multicast passing the filters */
  VMXNET3_RXM_BCAST = 0x04,  /**< Broadcast only */
  VMXNET3_RXM_ALL_MULTI = 0x08,  /**< All multicast */
  VMXNET3_RXM_PROMISC = 0x10,  /**< Promiscuous */
};

/** Variable-length configuration descriptor */
struct vmxnet3_variable_config {
  uint32_t version;
  uint32_t length;
  uint64_t address;
} __attribute__ (( packed ));

/** Driver shared area */
struct vmxnet3_shared {
  /** Magic signature */
  uint32_t magic;
  uint32_t pad; // make devread align on 64bit boundary
  /** Miscellaneous configuration */
  struct vmxnet3_misc_config misc;
  /** Interrupt configuration */
  struct vmxnet3_interrupt_config interrupt;
  /** Receive filter configuration */
  struct vmxnet3_rx_filter_config rx_filter;
  /** RSS configuration */
  struct vmxnet3_variable_config rss;
  /** Pattern-matching configuration */
  struct vmxnet3_variable_config pattern;
  /** Plugin configuration */
  struct vmxnet3_variable_config plugin;
  /** Event notifications */
  uint32_t ecr;
  /** Reserved */
  uint32_t reserved1[5];
} __attribute__ (( packed ));

/** Alignment of driver shared area */
#define VMXNET3_SHARED_ALIGN 8

/** Driver shared area magic */
#define VMXNET3_SHARED_MAGIC 0xbabefee1

/** Transmit descriptor */
struct vmxnet3_tx_desc {
  /** Address */
  uint64_t address;
  /** Flags */
  uint32_t flags[2];
} __attribute__ (( packed ));

/** Transmit generation flag */
#define VMXNET3_TXF_GEN 0x00004000UL

/** Transmit end-of-packet flag */
#define VMXNET3_TXF_EOP 0x000001000UL

/** Transmit completion request flag */
#define VMXNET3_TXF_CQ 0x000002000UL

/** Transmit completion descriptor */
struct vmxnet3_tx_comp {
  /** Index of the end-of-packet descriptor */
  uint32_t index;
  /** Reserved */
  uint32_t reserved0[2];
  /** Flags */
  uint32_t flags;
} __attribute__ (( packed ));

/** Transmit completion generation flag */
#define VMXNET3_TXCF_GEN 0x80000000UL

/** Transmit queue control */
struct vmxnet3_tx_queue_control {
  uint32_t num_deferred;
  uint32_t threshold;
  uint64_t reserved0;
} __attribute__ (( packed ));

/** Transmit queue configuration */
struct vmxnet3_tx_queue_config {
  /** Descriptor ring address */
  uint64_t desc_address;
  /** Data ring address */
  uint64_t immediate_address;
  /** Completion ring address */
  uint64_t comp_address;
  /** Driver-private data address */
  uint64_t driver_data_address;
  /** Reserved */
  uint64_t reserved0;
  /** Number of descriptors */
  uint32_t num_desc;
  /** Number of data descriptors */
  uint32_t num_immediate;
  /** Number of completion descriptors */
  uint32_t num_comp;
  /** Driver-private data length */
  uint32_t driver_data_len;
  /** Interrupt index */
  uint8_t intr_index;
  /** Reserved */
  uint8_t reserved[7];
} __attribute__ (( packed ));

/** Transmit queue statistics */
struct vmxnet3_tx_stats {
  /** Reserved */
  uint64_t reserved[10];
} __attribute__ (( packed ));

/** Receive descriptor */
struct vmxnet3_rx_desc {
  /** Address */
  uint64_t address;
  /** Flags */
  uint32_t flags;
  /** Reserved */
  uint32_t reserved0;
} __attribute__ (( packed ));

/** Receive generation flag */
#define VMXNET3_RXF_GEN 0x80000000UL

/** Receive completion descriptor */
struct vmxnet3_rx_comp {
  /** Descriptor index */
  uint32_t index;
  /** RSS hash value */
  uint32_t rss;
  /** Length */
  uint32_t len;
  /** Flags */
  uint32_t flags;
} __attribute__ (( packed ));

/** Receive completion generation flag */
#define VMXNET3_RXCF_GEN 0x80000000UL

/** Receive queue control */
struct vmxnet3_rx_queue_control {
  uint8_t update_prod;
  uint8_t reserved0[7];
  uint64_t reserved1;
} __attribute__ ((packed));

/** Receive queue configuration */
struct vmxnet3_rx_queue_config {
  /** Descriptor ring addresses */
  uint64_t desc_address[2];
  /** Completion ring address */
  uint64_t comp_address;
  /** Driver-private data address */
  uint64_t driver_data_address;
  /** Data ring base address */
  uint64_t data_ring_address;
  /** Number of descriptors */
  uint32_t num_desc[2];
  /** Number of completion descriptors */
  uint32_t num_comp;
  /** Driver-private data length */
  uint32_t driver_data_len;
  /** Interrupt index */
  uint8_t intr_index;
  uint8_t pad1;
  /** Rx data ring buffer size */
  uint16_t data_ring_size;
  /** Reserved */
  uint8_t reserved[4];
} __attribute__ ((packed));

/** Receive queue statistics */
struct vmxnet3_rx_stats {
  /** Reserved */
  uint64_t reserved[10];
} __attribute__ ((packed));

/** Queue status */
struct vmxnet3_queue_status {
  uint8_t stopped;
  uint8_t padding[3];
  uint32_t error;
} __attribute__ ((packed));

/** Transmit queue descriptor */
struct vmxnet3_tx_queue {
  struct vmxnet3_tx_queue_control ctrl;
  struct vmxnet3_tx_queue_config cfg;
  struct vmxnet3_queue_status status;
  struct vmxnet3_tx_stats state;
  uint8_t reserved[88];
} __attribute__ ((packed));

/** Receive queue descriptor */
struct vmxnet3_rx_queue {
  struct vmxnet3_rx_queue_control ctrl;
  struct vmxnet3_rx_queue_config cfg;
  struct vmxnet3_queue_status status;
  struct vmxnet3_rx_stats stats;
  uint8_t reserved[88];
} __attribute__ ((packed));

/**
 * Queue descriptor set
 *
 * We use only a single TX and RX queue
 */
struct vmxnet3_queues {
  /** Transmit queue descriptor(s) */
  struct vmxnet3_tx_queue tx;
  /** Receive queue descriptor(s) */
  struct vmxnet3_rx_queue rx[vmxnet3::NUM_RX_QUEUES];
} __attribute__ ((packed));
