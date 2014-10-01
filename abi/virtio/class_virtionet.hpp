/**  
     STANDARD:      
     Virtio 1.0, OASIS Committee Specification Draft 01
     (http://docs.oasis-open.org/virtio/virtio/v1.0/csd01/virtio-v1.0-csd01.pdf)
     
     In the following abbreviated to Virtio 1.01
*/
     

#ifndef CLASS_VIRTIONET_H
#define CLASS_VIRTIONET_H


#include <common>
#include <class_pci_device.hpp>
#include <virtio/class_virtio.hpp>

// From Virtio 1.01, 5.1.4
#define VIRTIO_NET_S_LINK_UP  1
#define VIRTIO_NET_S_ANNOUNCE 2

/** Virtio-net device driver.  */
class VirtioNet : Virtio {

  PCI_Device* dev;
  
  const Virtio::Queue& rx_q;
  const Virtio::Queue& tx_q;
  
  // From Virtio 1.01, 5.1.4
  struct config{
    mac_t mac = {0};
    uint16_t status;
  }_conf;
  
  char* _mac_str=(char*)"00:00:00:00:00:00";
  int _irq = 0;
  
  void irq_handler();

  
public: 
  const char* name();  
  const mac_t& mac();
  const char* mac_str();
  
  VirtioNet(PCI_Device* pcidev);


};

#endif
