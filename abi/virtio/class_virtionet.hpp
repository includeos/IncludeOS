#ifndef CLASS_VIRTIONET_H
#define CLASS_VIRTIONET_H


#include <common>
#include <class_pci_device.hpp>
#include <virtio/class_virtio.hpp>

#define VIRTIO_NET_S_LINK_UP    1              // Link is up

/** Possible driver types */
class VirtioNet : Virtio {

  PCI_Device* dev;
  
  //virtio_queue rx_q;
  //virtio_queue tx_q;
  
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
