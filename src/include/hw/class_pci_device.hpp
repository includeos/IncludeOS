#ifndef CLASS_PCI_DEVICE_HPP
#define CLASS_PCI_DEVICE_HPP

#include <hw/pci.h>
#include <hw/class_device.hpp>

#define PCI_WTF 0xffffffff

  
class PCI_Device: public Device{  

  //The 2-part PCI Bus address (The bus nr. is on Device)

  union pci_msg{
    uint32_t data;
    struct __attribute__((packed)){
      //Ordered low to high                                                           
      uint8_t msg;
      uint16_t addr;
      uint8_t code;
    };
  };
  
  //The 2-part PCI address
  uint16_t pci_addr;
  
  //The parts derived (if needed)
  uint8_t devno=0;
  uint8_t funcno=0;
  
  //The 2-part ID retrieved from the device
  union vendor_product{
    uint32_t __value;
    struct __attribute__((packed)){
      uint16_t vendor;
      uint16_t product;
    };
  } device_id;

  //The class code (device type)
  uint32_t classcode;

  //Printable names
  const char *classname;
  const char *vendorname;
  const char *productname;
  
  //The probing function (HW access)
  static inline uint32_t read_dword(uint16_t pci_addr, uint8_t msg);
  
  //Private constructor; only called if "Create" finds a device on this addr.
  PCI_Device(uint16_t pci_addr,uint32_t _id);
  
public:
  //Probe for a device on the given address
  static PCI_Device* Create(uint16_t pci_addr);  

  //Get a device by address
  static PCI_Device* get(uint16_t pci_addr);

  //Get a device by individual address parts
  static PCI_Device* get(int busno, int devno,int funcno);
};



/*
  TODO: Subclases, something like this.
 */

class Virtio : public PCI_Device{
  int irq;
  int iobase;
  unsigned long features;
  
};

class Nic : public PCI_Device{
  char* rxbuf;
  char* txbuf;
};

class Virtio_Nic : public Device, public Nic {
  
};



#endif
