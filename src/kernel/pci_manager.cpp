#include <assert.h>
#include <common>

#include <pci_manager.hpp>
#include <pci_device.hpp>



#define NUM_BUSES 2;


std::unordered_map<PCI::classcode_t, std::vector<PCI_Device> > PCI_manager::devices;

void PCI_manager::init()
{
  INFO("PCI Manager","Probing PCI bus");
  
  /* 
     Probe the PCI bus
     - Assuming bus number is 0, there are 255 possible addresses  
  */
  uint32_t id = PCI::WTF;
  
  for (uint16_t pci_addr = 0; pci_addr < 255; pci_addr++)
    {
      id = PCI_Device::read_dword(pci_addr, PCI::CONFIG_VENDOR);
      if (id != PCI::WTF)
	{
	  
	  PCI_Device dev (pci_addr,id);
	  devices[dev.classcode()].emplace_back(dev);
	}
      
    }
  
  // Pretty printing, end of device tree
  // @todo should probably be moved, or optionally non-printed
  INFO2("|");
  INFO2("o");
  
}
