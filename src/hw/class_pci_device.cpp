#include <hw/class_pci_device.hpp>

//PCI Device implementation
uint32_t PCI_Device::read_dword(uint16_t bus_address, uint8_t msg){
  pci_msg req;
  req.data=0x80000000;
  req.addr=bus_address;
  req.msg=msg;

  outpd(PCI_CONFIG_ADDR,(uint32_t)0x80000000 | req.data );
  return inpd(PCI_CONFIG_DATA);
}

//Private constructor; only called if "Create" finds a device on this addr.
PCI_Device::PCI_Device(uint16_t _pci_addr,uint32_t _id)
  : Device(Device::PCI),pci_addr(_pci_addr), device_id{_id}
{
  //We have device, so probe for details
  classcode=read_dword(_pci_addr,PCI_CONFIG_CLASS_REV);
  printf("\t * New PCI Device: Vendor: 0x%x Prod: 0x%x Class: 0x%x\n", 
         device_id.vendor,device_id.product,classcode);
  
  //TODO: Subclass this (or add it as member to a class) into device types.
  //...At least for NICs and HDDs
};

//Probe for a device on the given address (try to create)
PCI_Device* PCI_Device::Create(uint16_t pci_addr){ 
  
  //Probe PCI bus
  uint32_t devid=read_dword(pci_addr,PCI_CONFIG_VENDOR);

  if(devid==PCI_WTF) return 0;
  
  //TODO: Add this to a nice container first (now it gets lost)

  //Return new device
  return new PCI_Device(pci_addr, devid);
}
