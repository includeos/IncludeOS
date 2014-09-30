#ifndef CLASS_PCI_NIC_HPP
#define CLASS_PCI_NIC_HPP

#include <class_pci_device.hpp>
#include <stdio.h>



/*
class mac_t{
  unsigned char[6];
  const char* _str;
  
public:
  const char* c_str(){
    
  }
  }*/


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
  inline const char* name() { return driver.name(); }; 

  /** The actual mac address. */
  inline const mac_t& mac() { return driver.mac(); };
  
  /** Mac address string. */
  inline const char* mac_str() { return driver.mac_str(); };

  

    /** Event types */
    enum event_t {EthData, TCPConnection, TCPData, 
                UDPConnection, UDPData, HttpRequest};
  
  /** Attach event handlers to Nic-events. 
      
      @todo Decide between delegates and function pointers*/
  void on(event_t ev, void(*callback)());

private:

  DRIVER_T driver;
  
  /** Constructor. 
      
      Just a wrapper around the driver constructor.
      @note The Dev-class is a friend and will call this */
  Nic(PCI_Device* d): driver(d){};
  
  friend class Dev;

};


//Driver instead? And delegate everything to it with a delegate?

/**
class Virtio_Nic :  public Nic /{
  
};

**/

#endif //Class Nic
