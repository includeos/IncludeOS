#ifndef CLASS_PCI_NIC_HPP
#define CLASS_PCI_NIC_HPP

#include <hw/class_pci_device.hpp>

/** A public interface for Network cards
  
  @todo(Alfred): We should probably inherit something here, but I still don't know what the common denominators are between a Nic and other devices, except that they should have a "name". When do we ever need/want to treat all devices similarly? I don't want to introduce vtables/polymorphism unless I know it's really useful. */
class Nic{ 
  
public:
  /** Constructor. @todo How do we best keep users from calling this? */
  Nic(PCI_Device* d);
  
  /** Get a readable name. @todo Replace the dummy with something useful. */
  const char* name(); 

  /** Event types */
  enum event_t {EthData, TCPConnection, TCPData, 
                UDPConnection, UDPData, HttpRequest};
  
  /** Attach event handlers to Nic-events. 
      
      @todo Decide between delegates and function pointers*/
  void on(event_t ev, (void)(callback*)(...));

private:
  PCI_Device* pcidev;
  char* rxbuf;
  char* txbuf;  
  

};


#endif //Class Nic



//Driver instead? And delegate everything to it with a delegate?
class Virtio_Nic :  public Nic /* public Driver */{
  
};

