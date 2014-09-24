#ifndef CLASS_DEV_H
#define CLASS_DEV_H


#include <common>
#include <class_nic.hpp>
//#include <class_pci_manager.hpp>

/** @todo Impement */
class Disk;
class Serial;
class PIT;
class APIC;


//#define LEN_DEVNAME 50


/** The type of Nic to use. 

Any number of drivers can be created, but only one is implemnted atm.  */
typedef Nic<VirtioNet> Nic_t;
//typedef Nic<E1000> Nic_t;


/**
   Access point for devices
  
   Get a nic by calling `Dev::eth(n)`, a disk by calling `Dev::disk(n)` etc.
*/
class Dev{

public:
    
   
  //! Get ethernet device n
  static Nic_t& eth(int n);
  
  
  //! Get disk n
  static Disk& disk(int n);  

  //! Get serial port n
  static Serial& serial(int n);


private: 
  //Private pointer to the device lists
  static Nic_t* nics[MAX_NICS];
  
  static Disk* disks[MAX_DISKS];
  static Serial* serials[MAX_SERIALS];
  
  
  static void init();

  friend class OS;


    
};



/*
  A generic getter, in case we wanted two nics of different types.
  //! Get ethernet device n
  template<class DRIVER>
  static Nic<DRIVER>& eth(int n){
    if (n >= MAX_NICS)
      panic("Ethernet device not found!");
    
    PCI_Device* pcidev = PCI_manager::nic(n);
    
    if (!pcidev)
      panic("No PCI device found for nic!");
    
    if (!nics[n])
      nics[n] = new Nic<DRIVER>(pcidev);
    
    return *nics[n];
  };
  


 */


#endif

