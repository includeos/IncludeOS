#include <dev.hpp>
#include <pci_manager.hpp>

//int Device::busno(){return busnumber;}

/*Device::Device(bus_t _bustype, int _busno)
  : bustype(_bustype),busnumber(_busno){};*/


/*
Nic<E1000>& Dev::eth(int n){
  if(n>=MAX_NICS)
    panic("Ethernet device not found!");
  
  PCI_Device* pcidev=PCI_manager::nic(n);
  
  if(!pcidev)
    panic("No PCI device found for nic!");
  
  if(!nics[n])
    nics[n]=new Nic<E1000>(pcidev);

  return *nics[n];
}

*/

/*
void Dev::add(Nic<E1000>* nic){
  int i=0;

  //Register device
  while(i<MAX_NICS-1 and nics[i]) i++;
  
  if(nics[i]) //Last spot taken
    panic("Too many nics! \n");
  
  nics[i]=nic;
  printf("\n>>> Nic available at Device::eth(%i)\n",i);

  }*/

