#include <modern_virtio/control_plane.hpp>
#include <os.hpp>
#include <info>
#include <hw/pci.hpp>

// TODO: Replace INFO with debug that can be enabled or disabled

Virtio_control::Virtio_control(hw::PCI_Device& dev) :
  _pcidev(dev), _virtio_device_id(dev.product_id()), _msix_enabled(false)
{
  INFO("Virtio","Attaching to  PCI addr 0x%x",dev.pci_addr());

  /*
    Match vendor ID and Device ID in accordance with §4.1.2.2
  */
  bool vendor_is_virtio = (dev.vendor_id() == PCI::VENDOR_VIRTIO);
  CHECK(vendor_is_virtio, "Vendor ID is VIRTIO");
  _virtio_panic(vendor_is_virtio);

  _STD_ID = _virtio_device_id >= 0x1040 and _virtio_device_id < 0x107f;
  _LEGACY_ID = _virtio_device_id >= 0x1000 and _virtio_device_id <= 0x103f;

  CHECK(not _LEGACY_ID, "Device ID 0x%x is not legacy", _virtio_device_id);
  _virtio_panic(not _LEGACY_ID);

  CHECK(_STD_ID, "Device ID 0x%x is in valid range", _virtio_device_id);
  _virtio_panic(_STD_ID);

  /*
    Match Device revision ID. Virtio Std. §4.1.2.2
  */
  bool rev_id_ok = _version_supported(dev.rev_id());

  CHECK(rev_id_ok, "Device Revision ID (%d) supported", dev.rev_id());
  _virtio_panic(rev_id_ok);

  /* Finding Virtio structures */
  _find_cap_cfgs();

  /*
    Initializing the device. Virtio Std. §3.1
  */
  _reset();
  CHECK(true, "Resetting Virtio device");

  _set_ack_and_driver_bits();
  CHECK(true, "Setting acknowledgement and drive bits");
}

void Virtio_control::deactivate_virtio_control() {
  INFO("Virtio", "Deactivating control plane for Virtio device");

  _reset();
  if(_msix_enabled) {
    _pcidev.deactivate_msix();
  }
}

void Virtio_control::_find_cap_cfgs() {
  uint16_t status = _pcidev.read16(PCI_STATUS_REG);

  if ((status & 0x10) == 0) return;

  uint32_t offset = _pcidev.read32(PCI_CAPABILITY_REG) & 0xfc;

  /* Must be device vendor specific capability */
  while (offset) {
    uint32_t data    = _pcidev.read32(offset);
    uint8_t cap_vndr = static_cast<uint8_t>(data & 0xff);
    uint8_t cap_len  = static_cast<uint8_t>((data >> 16) & 0xff);
    uint8_t cfg_type = static_cast<uint8_t>(data >> 24);

    /* Skipping other than vendor specific capability */
    if (
      cap_vndr == PCI_CAP_ID_VNDR
    ) {
      /* Grabbing bar region */
      uint8_t bar        = static_cast<uint8_t>(_pcidev.read16(offset + VIRTIO_PCI_CAP_BAR) & 0xff);
      uint32_t bar_value = _pcidev.read32(PCI::CONFIG_BASE_ADDR_0 + (bar << 2));

      /* Grabbing bar offset for config */
      uint64_t bar_region = static_cast<uint64_t>(bar_value & ~0xf);
      uint64_t bar_offset = _pcidev.read32(offset + VIRTIO_PCI_CAP_BAROFF);

      /* Check if 64 bit bar */
      if (cap_len > VIRTIO_PCI_NOT_CAP_LEN) {
        uint64_t bar_hi    = static_cast<uint64_t>(_pcidev.read32(PCI::CONFIG_BASE_ADDR_0 + ((bar + 1) << 2)));
        uint64_t baroff_hi = static_cast<uint64_t>(_pcidev.read32(offset + VIRTIO_PCI_CAP_BAROFF64));

        bar_region |= (bar_hi << 32);
        bar_offset |= (baroff_hi << 32);
      }

      /* Determine config type and calculate config address */
      uint64_t cfg_addr = bar_region + bar_offset;

      switch(cfg_type) {
        case VIRTIO_PCI_CAP_COMMON_CFG:
          _common_cfg = reinterpret_cast<volatile virtio_pci_common_cfg*>(cfg_addr);
          break;
        case VIRTIO_PCI_CAP_DEVICE_CFG:
          _specific_cfg = cfg_addr;
          break;
        case VIRTIO_PCI_CAP_NOTIFY_CFG:
          _notify_region = reinterpret_cast<uint8_t*>(cfg_addr);
          _notify_off_multiplier = _pcidev.read32(offset + VIRTIO_PCI_NOTIFY_CAP_MUL);
          break;
      }
    }

    offset = (data >> 8) & 0xff;
  }
}

void Virtio_control::_reset() {
  _common_cfg->device_status = 0;
}

void Virtio_control::_set_ack_and_driver_bits() {
  _common_cfg->device_status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
  _common_cfg->device_status |= VIRTIO_CONFIG_S_DRIVER;
}

uint64_t Virtio_control::negotiate_features(
  uint64_t required_feats, 
  uint64_t optional_feats
) {
  /* Virtio version 1 is required for modern Virtio PCI */
  required_feats |= VIRTIO_F_VERSION_1;

  uint32_t required_feats_lo = static_cast<uint32_t>(required_feats & 0xffffffff);
  uint32_t required_feats_hi = static_cast<uint32_t>(required_feats >> 32);

  uint32_t optional_feats_lo = static_cast<uint32_t>(optional_feats & 0xffffffff);
  uint32_t optional_feats_hi = static_cast<uint32_t>(optional_feats >> 32);

  /* Read device features */
  _common_cfg->device_feature_select = 0;
  uint32_t dev_features_lo = _common_cfg->device_feature;

  _common_cfg->device_feature_select = 1;
  uint32_t dev_features_hi = _common_cfg->device_feature;

  /* Checking if required features are satisfied */
  bool satisfied_req_feats_lo = (dev_features_lo & required_feats_lo) == required_feats_lo;
  bool satisfied_req_feats_hi = (dev_features_hi & required_feats_hi) == required_feats_hi;
  bool satisfied_required_feats = satisfied_req_feats_lo and satisfied_req_feats_hi;
  CHECK(satisfied_required_feats, "Required features are satisfied");
  _virtio_panic(satisfied_required_feats);

  /* Checking for optional features */
  bool satisfied_opt_feats_lo = dev_features_lo & optional_feats_lo;
  bool satisfied_opt_feats_hi = dev_features_hi & optional_feats_hi;

  /* Supplying negotiated features */
  uint32_t nego_feats_lo = required_feats_lo | satisfied_opt_feats_lo;
  uint32_t nego_feats_hi = required_feats_hi | satisfied_opt_feats_hi;

  /* Writing subset of supported features */
  _common_cfg->driver_feature_select = 0;
  _common_cfg->driver_feature = nego_feats_lo;

  _common_cfg->driver_feature_select = 1;
  _common_cfg->driver_feature = nego_feats_hi;

  /* Writing features_ok status bit */
  _common_cfg->device_status |= VIRTIO_CONFIG_S_FEATURES_OK;

  /* Checking if features_ok bit is still set */
  bool features_ok = (_common_cfg->device_status & VIRTIO_CONFIG_S_FEATURES_OK) > 0;
  CHECK(features_ok, "Features OK bit is still set");
  _virtio_panic(features_ok);

  // Returning all the optional features supported
  return (static_cast<uint64_t>(satisfied_opt_feats_hi) << 32) 
    | satisfied_opt_feats_lo;
}

uint8_t Virtio_control::enable_msix(uint8_t required_msix_count) {
  if (required_msix_count > 0) {
    _msix_enabled = true;

    /* Grabbing PCI resources. Dependency for MSIX code */
    _pcidev.probe_resources();
    _pcidev.parse_capabilities();

    /* Checking for the availablity of MSIX capability */
    bool supports_msix = _pcidev.msix_cap() > 0;
    CHECK(supports_msix, "Device supports MSIX!");
    _virtio_panic(supports_msix);

    /* Driver requires required_msix_count # of vectors */
    _pcidev.init_msix();
    uint8_t msix_vector_count = _pcidev.get_msix_vectors();
    bool sufficient_msix_count = msix_vector_count >= required_msix_count;
    CHECK(sufficient_msix_count, "Sufficient msix vector count (%d)", msix_vector_count);
    _virtio_panic(sufficient_msix_count);

    return msix_vector_count;
  }

  return 0;
}

void Virtio_control::_virtio_panic(bool condition) {
  if (not condition) {
    _common_cfg->device_status |= VIRTIO_CONFIG_S_FAILED;

    os::panic("Virtio device failed!");
  }
}

void Virtio_control::set_driver_ok_bit() {
  INFO("Virtio", "Setting driver ok bit");
  _common_cfg->device_status |= VIRTIO_CONFIG_S_DRIVER_OK;

  /* Checking if device is still OK */
  if ((_common_cfg->device_status & VIRTIO_CONFIG_S_NEEDS_RESET)) {
    os::panic("Failure! Virtio device given up on guest");
  }
}