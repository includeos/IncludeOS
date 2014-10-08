#ifndef CLASS_PCI_NIC_HPP
#define CLASS_PCI_NIC_HPP

#include <class_pci_device.hpp>
#include <stdio.h>

// The nic knows about the IP stack
#include <net/class_ethernet.hpp>
#include <net/class_arp.hpp>
#include <net/class_ip4.hpp>
#include <net/class_ip6.hpp>


/** Future drivers may start out like so, */
class E1000{
public: 
  const char* name(){ return "E1000 Driver"; }
  //...whatever the Nic class implicitly needs
  
};

/** Or so, etc. 
    
    They'll have to provide anything the Nic class calls from them.
    @note{ There's no virtual interface, we'll need to call every function from 
    he Nic class in order to make sure all members exist }
 */
class RTL8139;


/** A public interface for Network cards
    
    We're using compile-time polymorphism for now, so the "inheritance" 
    requirements for a driver is implicily given by how it's used below,
    rather than explicitly by proper inheritance.
 */
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
  
  /** An IP stack (a skeleton for now).
      
      @todo We might consider adding the stack from the outside to save some 
      overhead for services who only wants a few layers. (I.e. a bridge
      might not need the whole stack) */              
  Ethernet _eth;
  Arp _arp;
  IP4 _ip4;
  IP6 _ip6;

  
  DRIVER_T driver;
  
  /** Constructor. 
      
      Just a wrapper around the driver constructor.
      @note The Dev-class is a friend and will call this */
  Nic(PCI_Device* d): 
    
    // Hook up the IP stack 
    _eth(), _arp(_eth),_ip4(_eth),_ip6(_eth),
    
    // Add PCI and ethernet layer to the driver
    driver(d,_eth)
  {};
  
  friend class Dev;

};

#endif //Class Nic
