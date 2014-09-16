#include <assert.h>
#include <hw/pci.h>

#include <class_pci_manager.hpp>
#include <hw/class_pci_device.hpp>
#include <virtio/virtio.h>

#define NUM_BUSES 2;
bus* buses=0;
unit* units=0;

const int MAX_UNITS=10;
unit unitlist[MAX_UNITS];
int unitcount=0;



extern "C" {
  int virtio_install(struct unit *unit, char *opts);
}

void PCI_manager::init(){
  printf("\n>>> PCI Manager initializing \n");
  
  /* 
     Probe the PCI bus (much simplified with OOP, yes?)
     - Assuming bus number is 0, there are 255 possible addresses  
  */     
  for(uint16_t pci_addr=0;pci_addr<256; pci_addr++)
    PCI_Device::Create(pci_addr);
    
  //PORTING virtio: this is not supposed to work yet.
  //virtio_install(&unitlist[virtionet],"some_opts" );
}
