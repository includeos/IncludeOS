#ifndef CLASS_PCI_DEVICE_HPP
#define CLASS_PCI_DEVICE_HPP

#include <hw/pci.h>
#include <hw/class_device.hpp>

#define PCI_WTF 0xffffffff

/**
   \brief Parent class for all PCI devices
   
   All low level communication with PCI devices should go here.
*/
class PCI_Device
  //:public Device //Why not? A PCI device is too general to be accessible?
{  

  //The 2-part PCI Bus address (The bus nr. is on Device)

  union pci_msg{
    uint32_t data;
    struct __attribute__((packed)){
      //Ordered low to high                                                           
      uint8_t reg;
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
  union class_revision{
    uint32_t reg;
    struct __attribute__((packed)){
      uint8_t rev_id;
      uint8_t prog_if;
      uint8_t subclass;
      uint8_t classcode;
    };
    struct __attribute__((packed)){
      uint16_t class_subclass;
      uint8_t __prog_if;
      uint8_t revision;        
    };
  }devtype;

  
  //Printable names
  const char *classname;
  const char *vendorname;
  const char *productname;
  
  /*
    Device Resources
   */
  
  //@brief Resource types, "Memory" or "I/O"
  enum resource_t{RES_MEM,RES_IO};
  
  //@brief A resource - possibly a list
  template<resource_t RT>
  struct Resource{
    const resource_t type = RT;
    uint32_t start_;
    uint32_t len_;
    Resource<RT>* next = 0;
    Resource<RT>(uint32_t start,uint32_t len):start_(start),len_(len){};
  };

  //@brief Resource lists. Members added by add_resource();
  Resource<RES_MEM>* res_mem_ = 0;
  Resource<RES_IO>* res_io_ = 0;
   
   
  //@brief Read from device with implicit pci_address (e.g. used by Nic)
  inline uint32_t read_dword(uint8_t reg){
    pci_msg req;
    req.data=0x80000000;
    req.addr=pci_addr;
    req.reg=reg;
    
    outpd(PCI_CONFIG_ADDR,(uint32_t)0x80000000 | req.data );
    return inpd(PCI_CONFIG_DATA);
  };

  //@brief Write to device with implicit pci_address (e.g. used by Nic)
  inline void write_dword(uint8_t reg,uint32_t value){
    pci_msg req;
    req.data=0x80000000;
    req.addr=pci_addr;
    req.reg=reg;
    
    outpd(PCI_CONFIG_ADDR,(uint32_t)0x80000000 | req.data );
    outpd(PCI_CONFIG_DATA, value);
  };

  //@brief add a resource to a resource queue
  //(This seems pretty dirty; private class, reference to pointer etc.)
  template<resource_t R_T>
  void add_resource(Resource<R_T>* res,Resource<R_T>*& Q){
    Resource<R_T>* q;
    if (Q) {
      q = Q;
      while (q->next) q=q->next;
      q->next=res;
    } else {
      printf("Q is now res \n");
      Q=res;
    }
      
  };

public:
  /*
    Static functions
  */  
  
  //@brief Read from device with explicit pci_addr
  static inline uint32_t read_dword(uint16_t pci_addr, uint8_t reg){
    pci_msg req;
    req.data=0x80000000;
    req.addr=pci_addr;
    req.reg=reg;
    
    outpd(PCI_CONFIG_ADDR,(uint32_t)0x80000000 | req.data );
    return inpd(PCI_CONFIG_DATA);
  };  

  //

  //DROP: We got performance degradation when using this in the probing
  //Probe for a device on the given address
  static PCI_Device* Create(uint16_t pci_addr);  

  //Get a device by address
  static PCI_Device* get(uint16_t pci_addr);

  //Get a device by individual address parts
  static PCI_Device* get(int busno, int devno,int funcno);
  
  
  /*
    Member functions
   */  
  //@brief PCI Device Constructor
  PCI_Device(uint16_t pci_addr,uint32_t _id);

  //@brief A descriptive name
  const char* name();
  
  //@brief Get the PCI address (composite of bus, device and function)
  uint16_t get_pci_addr();
    
  //@brief Parse all Base Address Registers (BAR's)
  void probe_resources();
  
  //@brief the base address of the (first) I/O resource
  uint32_t iobase();
  
  
};



/*
  TODO: Subclases, something like this.
 */

class Virtio : public PCI_Device{
  int irq;
  int iobase;
  unsigned long features;
  
};


#endif
