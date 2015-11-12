#include <assert.h>
#include <common>

#include <pci_manager.hpp>
#include <pci_device.hpp>
#include <nic.hpp>


#define NUM_BUSES 2;
//bus* buses=0;
//unit* units=0;

static const int MAX_UNITS = 10;
//static int unitcount = 0;
//unit unitlist[MAX_UNITS];


extern "C" {
  int virtio_install(struct unit *unit, char *opts);
}

PCI_Device* PCI_manager::nics[MAX_Q];
PCI_Device* PCI_manager::disks[MAX_Q];


void PCI_manager::init()
{
  printf(">>> PCI Manager initializing \n");
  
  /* 
     Probe the PCI bus
     - Assuming bus number is 0, there are 255 possible addresses  
  */
  uint32_t id = PCI_WTF;
  PCI_Device* dev = nullptr;
  
  for (uint16_t pci_addr = 0; pci_addr < 255; pci_addr++)
  {
    id = PCI_Device::read_dword(pci_addr, PCI_CONFIG_VENDOR);
    if (id != PCI_WTF)
    {
      dev = new PCI_Device(pci_addr,id);
      
      // Keep the devices we might need
      switch (dev->classcode())
      {
      case CL_NIC:
          add<nics>(dev);
          break;
      case CL_STORAGE:
          add<disks>(dev);
          break;
      default: break;
        // Add cases for others as needed
      }        
    }
  }
  
  // Pretty printing, end of device tree
  // @todo should probably be moved, or optionally non-printed
  printf("\t | \n");
  printf("\t o \n");
  
}
