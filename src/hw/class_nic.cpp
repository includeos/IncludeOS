#include <hw/class_nic.hpp>



const char* Nic::name(){
  return "Fantastic Nic No.1";
}
  
Nic::Nic(PCI_Device* _dev)
  : pcidev(_dev) //Device(this)
{
  
  printf("\n Nic at PIC addr 0x%x scanning for resources\n",_dev->get_pci_addr());
    
  _dev->probe_resources();
  
}
