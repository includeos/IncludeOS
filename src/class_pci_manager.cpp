#include <assert.h>
#include <hw/pci.h>

#include <class_pci_manager.hpp>
#include <class_pci_device.hpp>
#include <class_nic.hpp>
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

PCI_Device* PCI_manager::nics[MAX_Q];


void PCI_manager::init(){
  printf("\n>>> PCI Manager initializing \n");

  /* 
     Probe the PCI bus
     - Assuming bus number is 0, there are 255 possible addresses  
  */
  uint32_t id=PCI_WTF;
  PCI_Device* dev=0;
  for (uint16_t pci_addr = 0; pci_addr < 256; pci_addr++) {
    id = PCI_Device::read_dword(pci_addr,PCI_CONFIG_VENDOR);
    if (id != PCI_WTF) {
      dev = new PCI_Device(pci_addr,id);
      
      switch (dev->classcode()) {
      case CL_NIC : add<nics>(dev); break;
      default: break;
        //Add cases for other devices we want to keep.
      }        
    }
  }
  
  
  //printf("\n>>> Eth0: %s \n",Dev::eth<E1000>(0).name());
  
    
  //PORTING virtio: this is not supposed to work yet.
  //virtio_install(&unitlist[virtionet],"some_opts" );
  

}
