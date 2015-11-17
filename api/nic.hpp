#ifndef CLASS_PCI_NIC_HPP
#define CLASS_PCI_NIC_HPP

#include <pci_device.hpp>
#include <stdio.h>

#include <net/ethernet.hpp>
#include <net/inet_common.hpp>
#include <net/buffer_store.hpp>

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
template<class DRIVER>
class Nic{ 
   
public:

  typedef DRIVER driver; 
  
  /** Get a readable name. @todo Replace the dummy with something useful.*/
  //const char* name(); 
  
  /** @note If we add this while there's a specialization, this overrides. */
  inline const char* name() { return _driver.name(); }

  /** The actual mac address. */
  inline const net::Ethernet::addr mac() { return _driver.mac(); }
  
  /** Mac address string. */
  inline const char* mac_str() { return _driver.mac_str(); }

  inline void set_linklayer_out(net::upstream del)
  { _driver.set_linklayer_out(del); }
  
  inline int transmit(net::Packet_ptr pckt)
  { return _driver.transmit(pckt); }
  
  inline uint16_t MTU () const 
  { return _driver.MTU(); }
  
  inline net::BufferStore& bufstore()
  { return _driver.bufstore(); }
  
private:
  
  DRIVER _driver;

  
  /** Constructor. 
      
      Just a wrapper around the driver constructor.
      @note The Dev-class is a friend and will call this */
  Nic(PCI_Device& d): _driver(d){}
  
  friend class Dev;

};

#endif //Class Nic
