#pragma once

#include <vector>
#include <unordered_map>

#include <pci_device.hpp>

#include <stdio.h>
#include <syscalls.hpp>

  
class PCI_Device;

class PCI_manager{
  
private:
  
  /** Keep track of certain devices 
      
      The PCI manager can probe and keep track of devices which can (possibly)
	be specialized by the Dev-class later. 
  */
  static void init();
  
  static std::unordered_map<PCI::classcode_t, std::vector<PCI_Device> > devices;
  
  
public:
  
  template <PCI::classcode_t CLASS>
  static PCI_Device& device(int n){
    
    return devices[CLASS][n];
    
  };    
  
  friend class OS;  
  
};
