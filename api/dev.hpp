#ifndef CLASS_DEV_H
#define CLASS_DEV_H


#include <common>
#include <pci_manager.hpp>
#include <nic.hpp>
#include <virtio/virtionet.hpp>

/** @todo Impement */
class Disk;
class Serial;
class PIT;
class APIC;


/**
   Access point for devices
  
   Get a nic by calling `Dev::eth(n)`, a disk by calling `Dev::disk(n)` etc.
*/
class Dev{

public:
      
  
  /** Get ethernet device n */
  template <int N, typename DRIVER = VirtioNet>
  static Nic<DRIVER>& eth(){ 
    static Nic<DRIVER> eth_( PCI_manager::device<PCI::NIC> (N) );
    return eth_;
  }
  
  /** Get disk n */
  template <typename DRIVER>
  static PCI_Device& disk(int n){
    
  }
  
  /** Get serial port n */
  template <typename DRIVER>
  static PCI_Device& serial(int n);

  static PIT& basic_timer();
  
};



#endif

