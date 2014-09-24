#ifndef CLASS_PCI_NIC_HPP
#define CLASS_PCI_NIC_HPP

#include <class_pci_device.hpp>
#include <stdio.h>

/** Possible driver types */
class VirtioNet{
public: const char* name(){ return "VirtioNet Driver"; };
};

class E1000{
public: const char* name(){ return "E1000 Driver"; };
};

class RTL8139;



/** A public interface for Network cards
  
  @todo(Alfred): We should probably inherit something here, but I still don't know what the common denominators are between a Nic and other devices, except that they should have a "name". When do we ever need/want to treat all devices similarly? I don't want to introduce vtables/polymorphism unless I know it's really useful. */
template<class DRIVER_T>
class Nic{ 
   
public:
 
  /** Get a readable name. @todo Replace the dummy with something useful.*/
  //const char* name(); 
  
  /** @note If we add this while there's a specialization, this overrides. */
  inline const char* name(){return driver.name();}; 

  /** Event types */
  enum event_t {EthData, TCPConnection, TCPData, 
                UDPConnection, UDPData, HttpRequest};
  
  /** Attach event handlers to Nic-events. 
      
      @todo Decide between delegates and function pointers*/
  void on(event_t ev, void(*callback)());

private:

  PCI_Device* pcidev;
  DRIVER_T driver;
  char* rxbuf;
  char* txbuf;  
  
  /** Constructor. The Dev-class is a friend and can call this */
  
  Nic(PCI_Device* d) 
    : pcidev(d) //Device(this)
  {
    
    printf("\n Nic at PCI addr 0x%x scanning for resources\n",d->pci_addr());
    
    d->probe_resources();
  };
  
  friend class Dev;

};


//Driver instead? And delegate everything to it with a delegate?

/**
class Virtio_Nic :  public Nic /{
  
};

**/

#endif //Class Nic
