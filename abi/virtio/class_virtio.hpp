#ifndef CLASS_VIRTIO_HPP
#define CLASS_VIRTIO_HPP

#include <class_pci_device.hpp>

class Virtio{
  
  //PCI memer as reference (so no indirection overhead)
  PCI_Device& _pcidev;
  
  //We'll get this from PCI_device::iobase(), but that lookup takes longer
  uint32_t _iobase;  
  uint8_t _irq = 0;
  uint32_t _features;

  void set_irq();
  
public:
  /** Get the Virtio config registers from the PCI device.
      
      @note it varies how these are structured, hence a void* buf */
  void get_config(void* buf, int len);
  
  /** Get the (saved) device IRQ */
  inline uint8_t irq();

  /** Reset the virtio device */
  void reset();
  
  /** Signal "Driver found" */
  void sig_driver_found();

  /** Negotiate supported features with host */
  void negotiate_features(uint32_t features);
  
  /** Register interrupt handler & enable IRQ */
  void enable_irq_handler();

  /** Probe PCI device for features */
  uint32_t probe_features();
  
  /** Get locally stored features */
  uint32_t features();
  
  /** Get iobase. Wrapper around PCI_Device::iobase */
  inline uint32_t iobase(){ return _iobase; }

  /** Kick hypervisor.
   
      Will notify the host (Qemu/Virtualbox etc.) about pending data  */
  inline void kick();
  
  Virtio(PCI_Device* pci);
  
};

#endif

