#ifndef CLASS_PCI_NIC_HPP
#define CLASS_PCI_NIC_HPP

#include <hw/class_pci_device.hpp>

/*
  @brief A public interface for Network cards
  
  TODO(alfred): 
  We should probably inherit something here, but I still don't know what the common denominators are between a Nic and other devices, except that they should have a "name". When do we ever need/want to treat all devices similarly? I don't want to introduce vtables/polymorphism unless I know it's really useful.
*/

class Nic{ 
  PCI_Device* pcidev;
  char* rxbuf;
  char* txbuf;  
public:
  Nic(PCI_Device* d);
  const char* name(); //But never use polymorphically
};


#endif //Class Nic



//Driver instead? And delegate everything to it with a delegate?
class Virtio_Nic :  public Nic /* public Driver */{
  
};

