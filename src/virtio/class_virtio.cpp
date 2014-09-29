#include <virtio/class_virtio.hpp>
#include <syscalls.hpp>
#include <virtio/virtio.h>
#include <class_irq_handler.hpp>

void Virtio::set_irq(){
  
  //Get device IRQ 
  uint32_t value = _pcidev.read_dword(PCI_CONFIG_INTR);
  if ((value & 0xFF) > 0 && (value & 0xFF) < 32){
    _irq = value & 0xFF;    
  }
  
}

Virtio::Virtio(PCI_Device* dev)
  : _pcidev(*dev)
{
  printf("\n>>> Virtio attaching to  PCI addr 0x%x \n",_pcidev.pci_addr());
  
  if(_pcidev.vendor_id() != PCI_Device::VENDOR_VIRTIO)
    panic("This is not a Virtio device");
  
  
  printf("\t [x] Vendor ID is VIRTIO \n");
  
  //Fetch IRQ from PCI resource
  set_irq();
  printf(_irq ? "\t [x] Unit IRQ %i \n " : "\n [0] NO IRQ on device \n",_irq);

  _pcidev.probe_resources();
  _iobase=_pcidev.iobase();

  printf(_irq ? "\t [x] Unit I/O base 0x%lx \n " : 
         "\n [ ] NO I/O Base on device \n",_iobase);


  uint32_t queue_size = inpd(_iobase + 0x0C);
  
  printf(queue_size > 0 and queue_size != PCI_WTF ?
         "\t [x] Queue Size : 0x%lx \n" :
         "\t [ ] No qeuue Size? : 0x%lx \n" ,queue_size);

  // Do stuf in the order described in Virtio standard v.1, sect. 3.1:
  
  // 1. Reset
  reset();
  printf("\t [*] Reset device \n");
  
  // 2. Set driver status bit
  // TODO
  
  negotiate_features(0);
  printf("\t [*] Negotiate features \n");
  enable_irq_handler();
  printf("\t [*] Enable IRQ Handler \n");
  
  printf("\n  >> Virtio initialization complete \n\n");
  
  
  //OSdev lists this as a status field, but Ringaard does not.
  //uint32_t status1 = inpd(_iobase + 0x12);
  
}

void Virtio::get_config(void* buf, int len){
  unsigned char* ptr = (unsigned char*)buf;
  uint32_t ioaddr = _iobase + VIRTIO_PCI_CONFIG;
  int i;
  for (i = 0; i < len; i++) *ptr++ = inp(ioaddr + i);
}


void Virtio::reset(){
  outp(_iobase + VIRTIO_PCI_STATUS, 0);
}

void Virtio::sig_driver_found(){
  outp(_iobase + VIRTIO_PCI_STATUS, inp(_iobase + VIRTIO_PCI_STATUS) | VIRTIO_CONFIG_S_ACKNOWLEDGE | VIRTIO_CONFIG_S_DRIVER);

}


uint32_t Virtio::probe_features(){
  return inpd(_iobase + VIRTIO_PCI_HOST_FEATURES);
}

void Virtio::negotiate_features(uint32_t features){
  _features = inpd(_iobase + VIRTIO_PCI_HOST_FEATURES);
  _features &= features;
  outpd(_iobase + VIRTIO_PCI_GUEST_FEATURES, _features);
}


void irq_virtio_handler(){
    printf("VIRTIO IRQ! \n");
};


void Virtio::enable_irq_handler(){
  //_irq=0; //Works only if IRQ2INTR(_irq), since 0 overlaps an exception.
  //IRQ_handler::set_handler(IRQ2INTR(_irq), irq_virtio_entry);
 
  IRQ_handler::subscribe(_irq,irq_virtio_handler);
  IRQ_handler::enable_irq(_irq);
}




