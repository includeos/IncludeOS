#pragma once
#ifndef VIRTIO_VIRTIO_HPP
#define VIRTIO_VIRTIO_HPP

#include <hw/pci_device.hpp>
#include <vector>
#include <cstdint>

/* Virtio PCI capability */
typedef struct __attribute__((packed)) { 
  uint8_t cap_vndr;    /* Generic PCI field: PCI_CAP_ID_VNDR */ 
  uint8_t cap_next;    /* Generic PCI field: next ptr. */ 
  uint8_t cap_len;     /* Generic PCI field: capability length */ 
  uint8_t cfg_type;    /* Identifies the structure. */ 
  uint8_t bar;         /* Where to find it. */ 
  uint8_t id;          /* Multiple capabilities of the same type */ 
  uint8_t padding[2];  /* Pad to full dword. */ 
  uint32_t offset;     /* Offset within bar. */ 
  uint32_t length;     /* Length of the structure, in bytes. */ 
} virtio_pci_cap;

/* Virtio PCI capability 64 */
typedef struct __attribute__((packed)) {
  virtio_pci_cap cap;
  uint32_t offset_hi;
  uint32_t length_hi;
} virtio_pci_cap64;

typedef struct __attribute__((packed)) { 
  virtio_pci_cap cap; 
  uint32_t notify_off_multiplier; /* Multiplier for queue_notify_off. */ 
} virtio_pci_notify_cap;

#define VIRTIO_PCI_CAP_LEN     sizeof(virtio_pci_cap)
#define VIRTIO_PCI_CAP_LEN64   sizeof(virtio_pci_cap64)
#define VIRTIO_PCI_NOT_CAP_LEN sizeof(virtio_pci_notify_cap)

#define VIRTIO_PCI_CAP_BAR        offsetof(virtio_pci_cap, bar)
#define VIRTIO_PCI_CAP_BAROFF     offsetof(virtio_pci_cap, offset)
#define VIRTIO_PCI_CAP_BAROFF64   offsetof(virtio_pci_cap64, offset_hi)
#define VIRTIO_PCI_NOTIFY_CAP_MUL offsetof(virtio_pci_notify_cap, notify_off_multiplier)

typedef struct __attribute__((packed)) { 
  /* About the whole device. */ 
  volatile uint32_t device_feature_select;      /* read-write */ 
  volatile uint32_t device_feature;             /* read-only for driver */ 
  volatile uint32_t driver_feature_select;      /* read-write */ 
  volatile uint32_t driver_feature;             /* read-write */ 
  volatile uint16_t config_msix_vector;         /* read-write */ 
  volatile uint16_t num_queues;                 /* read-only for driver */ 
  volatile uint8_t device_status;               /* read-write */ 
  volatile uint8_t config_generation;           /* read-only for driver */ 
 
  /* About a specific virtqueue. */ 
  volatile uint16_t queue_select;              /* read-write */ 
  volatile uint16_t queue_size;                /* read-write */ 
  volatile uint16_t queue_msix_vector;         /* read-write */ 
  volatile uint16_t queue_enable;              /* read-write */ 
  volatile uint16_t queue_notify_off;          /* read-only for driver */ 
  volatile uint64_t queue_desc;                /* read-write */ 
  volatile uint64_t queue_driver;              /* read-write */ 
  volatile uint64_t queue_device;              /* read-write */ 
  volatile uint16_t queue_notif_config_data;   /* read-only for driver */ 
  volatile uint16_t queue_reset;               /* read-write */ 
 
  /* About the administration virtqueue. */ 
  volatile uint16_t admin_queue_index;         /* read-only for driver */ 
  volatile uint16_t admin_queue_num;           /* read-only for driver */ 
} virtio_pci_common_cfg;

/* Types of configurations */ 
#define VIRTIO_PCI_CAP_COMMON_CFG        1 
#define VIRTIO_PCI_CAP_NOTIFY_CFG        2 
#define VIRTIO_PCI_CAP_ISR_CFG           3 
#define VIRTIO_PCI_CAP_DEVICE_CFG        4 
#define VIRTIO_PCI_CAP_PCI_CFG           5 
#define VIRTIO_PCI_CAP_SHARED_MEMORY_CFG 8 
#define VIRTIO_PCI_CAP_VENDOR_CFG        9

/* All feats these must be supported by device */
#define VIRTIO_F_VERSION_1        (1ULL << 32)

/* Features to use if available */
#define VIRTIO_F_INDIRECT_DESC_LO (1ULL << 28)
#define VIRTIO_F_EVENT_IDX_LO     (1ULL << 29)

/* Other features that will not be supported */
#define VIRTIO_F_RING_PACKED_HI   (1ULL << 2)
#define VIRTIO_F_RING_RESET_HI    (1ULL << 8)
#define VIRTIO_F_IN_ORDER_HI      (1ULL << 3)

#define VIRTIO_CONFIG_S_ACKNOWLEDGE     1
#define VIRTIO_CONFIG_S_DRIVER          2
#define VIRTIO_CONFIG_S_DRIVER_OK       4
#define VIRTIO_CONFIG_S_FEATURES_OK     8
#define VIRTIO_CONFIG_S_NEEDS_RESET     64
#define VIRTIO_CONFIG_S_FAILED          128

/* 
  Virtio control plane implementation. Can be composed or inherited.
*/
class Virtio_control {
  public:
    Virtio_control(hw::PCI_Device& dev);

    /** Called by upper level driver to disable
     *  the main Virtio control plane. Upper level
     *  needs to deactivate the queues in its own manner. */
    void deactivate_virtio_control();

    /** Interface for upper layer to tell Virtio PCI about
     *  wanted and necessary features. Used during negotiation. 
     *  For now only features from bit 0 to 63 inclusive considered.
     *  Returns the negotiated optional features. 
     *  Panics if not all required features are available */
    uint64_t negotiate_features(uint64_t required_feats, uint64_t optional_feats);

    /** Requests some amount of required MSIX vectors. 
     *  Panics if not satisfied. 
     *  Returns the # of available MSIX vectors if satisfied
     *  Not useful if driver is not using interrupts */
    uint8_t enable_msix(uint8_t required_msix_count);

    /** Setting driver ok bit within device status */
    void set_driver_ok_bit();
    
    /** Modifying the device configuration */
    inline volatile virtio_pci_common_cfg& common_cfg() { return *_common_cfg; }
    inline uint64_t specific_cfg() { return _specific_cfg; }
    
    /** Queue notification information */
    inline uint32_t notify_off_multiplier() const { return _notify_off_multiplier; }
    inline uint8_t *notify_region() const { return _notify_region; }
    
  private:
    /** Finds the common configuration address */
    void _find_cap_cfgs();
    
    /** Reset the virtio device */
    void _reset();
  
    /** Setting acknowledgement and driver status bits within device status */
    void _set_ack_and_driver_bits();
    
    /** Panics if there is an assert error (condition is false).
     *  Unikernel should not continue further because device is a dependency.
     *  Write failed bit to device status.
     */
    void _virtio_panic(bool condition);
    
    /** Indicate which Virtio version (PCI revision ID) is supported.
     *  I am currently adding support for modern Virtio (1.3).
     *  This implementation only drives non-transitional modern Virtio devices.
     */
    inline bool _version_supported(uint16_t i) { return i == 1; }

    /* Configuration structures */
    volatile virtio_pci_common_cfg *_common_cfg;
    volatile uint64_t _specific_cfg; // specific to the device
    
    /* Offsets and offset multipliers */
    uint32_t _notify_off_multiplier;
    uint8_t *_notify_region;
    
    /* Indicate if virtio device ID is legacy or standard */
    bool _LEGACY_ID = 0;
    bool _STD_ID = 0;
    
    /* Other */
    hw::PCI_Device& _pcidev;
    bool _msix_enabled;
    uint16_t _virtio_device_id = 0;
};

#endif
