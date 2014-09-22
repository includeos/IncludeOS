#include <hw/class_device.hpp>

//int Device::busno(){return busnumber;}

/*Device::Device(bus_t _bustype, int _busno)
  : bustype(_bustype),busnumber(_busno){};*/

Nic* Dev::nics[MAX_NICS]={0}; //Should be zeroed by .bss ...?

Nic& Dev::eth(int n){
  if(n<MAX_NICS-1 and nics[n])
    return *nics[n];
  panic("Ethernet device not found!");
}


void Dev::add(Nic* nic){
  int i=0;

  //Register device
  while(i<MAX_NICS-1 and nics[i]) i++;
  
  if(nics[i]) //Last spot taken
    panic("Too many nics! \n");
  
  nics[i]=nic;
  printf("\n>>> Nic available at Device::eth(%i)\n",i);

}

