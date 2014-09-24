#include <class_nic.hpp>
#include <class_pci_device.hpp>





/*
template <> const char* Nic<VirtioNet>::name(){
  //return "Fantastic VirtioNic No.1";
  return driver.name();
}
*/

/*
template <> Nic<VirtioNet>::Nic(PCI_Device* _dev)
  : pcidev(_dev) //Device(this)
{
  
  printf("\n Nic at PCI addr 0x%x scanning for resources\n",_dev->pci_addr());
    
  _dev->probe_resources();
  
}

*/

/*
template <> const char* Nic<E1000>::name(){
  return "Specialized E1000 No.1";
}
*/

template <> Nic<E1000>::Nic(PCI_Device* _dev)
  : pcidev(_dev) //Device(this)
{
  
  printf("\n Nic at PCI addr 0x%x scanning for resources\n",_dev->pci_addr());
    
  _dev->probe_resources();
  
}
