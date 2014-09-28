#ifndef PCI_MANAGER_H
#define PCI_MANAGER_H

//#include <hw/pci.h>
#include <class_pci_device.hpp>

class PCI_Device;

#include <stdio.h>
#include <syscalls.hpp>


/*
unit* add_unit(bus* bus, unsigned long classcode, unsigned long unitcode, unsigned long unitno);

resource* add_resource(unit* unit, unsigned short type, unsigned short flags, unsigned long start, unsigned long len);

bus* add_bus(unit* self, unsigned long bustype, unsigned long busno);
*/

class PCI_manager{
  
private:
  
#define MAX_Q MAX_NICS
  
  /** Keep track of certain devices 
      
      The PCI manager can probe and keep track of devices which can (possibly)
      be specialized by the Dev-class later. 
  */
  static PCI_Device* nics[MAX_Q];
  static PCI_Device* disks[MAX_Q];
  
  template<PCI_Device* Q[]>
  static inline void add(PCI_Device* dev){
    for (int i=0; i<MAX_Q; i++)
      if(not Q[i]){
        Q[i]=dev;
        ::printf("\t |  +--[ SAVED ]\n"); 
        return;
      }
    
    panic("PCI Device queue full!");
  }

  static void init();

  static PCI_Device* nic(int n){
    return n < MAX_Q ? nics[n] : 0;
  };
  
  static PCI_Device* disk(int n){
    return n < MAX_Q ? disks[n] : 0;
  }

  
  friend class Dev;
  


public: 
    
  

};

#endif
