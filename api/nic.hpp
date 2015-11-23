#ifndef NIC_HPP
#define NIC_HPP

#include <pci_device.hpp>

#include <net/ethernet.hpp>
#include <net/inet_common.hpp>
#include <net/buffer_store.hpp>

/** A public interface for Network cards
    
    @note: The requirements for a driver is implicitly given by how it's used below,
    rather than explicitly by inheritance. This avoids vtables.
    
    @note: Drivers are passed in as template paramter so that only the drivers
    we actually need will be added to our project. 
 */
template <typename DRIVER>
class Nic{ 
   
public:

  typedef DRIVER driver; 

/** Get a readable name. */
  inline const char* name() { return _driver.name(); }

  /** The mac address. */
  inline const net::Ethernet::addr& mac() { return _driver.mac(); }
  

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
  Nic(PCI_Device& d): _driver(d) {}
  
  friend class Dev;

};

/** Future drivers may start out like so, */
class E1000{
public: 
  const char* name(){ return "E1000 Driver"; }
  //...whatever the Nic class implicitly needs
  
};

/** Hopefully somebody will port a driver for this one */
class RTL8139;

#endif // NIC_HPP
