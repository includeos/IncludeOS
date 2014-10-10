#ifndef CLASS_PCI_NIC_HPP
#define CLASS_PCI_NIC_HPP

#include <class_pci_device.hpp>
#include <stdio.h>

// The nic knows about the IP stack
#include <net/class_ip_stack.hpp>

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
  inline const char* name() { return _driver.name(); }; 

  /** The actual mac address. */
  inline const mac_t& mac() { return _driver.mac(); };
  
  /** Mac address string. */
  inline const char* mac_str() { return _driver.mac_str(); };

  

    /** Event types */
    enum event_t {EthData, TCPConnection, TCPData, 
                UDPConnection, UDPData, HttpRequest};
  
  /** Attach event handlers to Nic-events. 
      
      @todo Decide between delegates and function pointers*/
  void on(event_t ev, void(*callback)());

private:
  
  /** An IP stack (a skeleton for now).
      
      @todo We might want to construct this from the outside.*/
  IP_stack _net;
  
  DRIVER_T _driver;
  
  /** Constructor. 
      
      Just a wrapper around the driver constructor.
      @note The Dev-class is a friend and will call this */
  Nic(PCI_Device* d): 
    // Add PCI and ethernet layer to the driver
    _driver(d)
  {
    // Upstream
    auto stack_bottom=delegate<int(uint8_t*,int)>::from<IP_stack,
                                                    &IP_stack::physical_in>(_net);
    
    _driver.set_linklayer_out(stack_bottom);
    
    // Downstream
    auto driver_top=delegate<int(uint8_t*,int)>::from<DRIVER_T,
                                                      &DRIVER_T::linklayer_in>(_driver);    
    _net.set_physical_out(driver_top);
  };
  
  friend class Dev;

};

#endif //Class Nic
