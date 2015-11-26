#ifndef CLASS_DEV_H
#define CLASS_DEV_H


#include <common>
#include <pci_manager.hpp>
#include <nic.hpp>
#include <pit.hpp>
#include <disk.hpp>
#include <virtio/virtionet.hpp>

/** @todo Impement */
class Serial;
class APIC;
class HPET;

/**
   Access point for devices
  
   Get a nic by calling `Dev::eth<0,Virtio_Net>(n)`, a disk by calling `Dev::disk(n)` etc.
*/
class Dev{

public:
        
  /** Get ethernet device n */
  template <int N, typename DRIVER = VirtioNet>
  static Nic<DRIVER>& eth(){ 
    static Nic<DRIVER> eth_( PCI_manager::device<PCI::NIC> (N) );
    return eth_;
  }
  
  /** @todo Get disk n */
  template <int N, typename DRIVER>
  static PCI_Device& disk(int){
    static Disk<DRIVER> disk_( PCI_manager::device<PCI::STORAGE> (N) );
    return disk_;
  }
  
  /** Get serial port n. 
      @Todo: Make a serial port class, and move rsprint / rswrite etc. from OS out to it.
      @Note: The DRIVER parameter is there to support virtio serial ports. */
  template <typename DRIVER>
  static PCI_Device& serial(int n);  
  
  /** Programmable Interval Timer device, with ~ms-precision asynchronous timers.*/
  static PIT& basic_timer(){
    return PIT::instance();
  };
  
};



#endif

