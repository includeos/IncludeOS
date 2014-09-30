#include <virtio/class_virtionet.hpp>
#include <virtio/virtio.h>
#include <class_irq_handler.hpp>
#include <stdio.h>

const char* VirtioNet::name(){ return "VirtioNet Driver"; }
const mac_t& VirtioNet::mac(){ return _conf.mac; }  
const char* VirtioNet::mac_str(){ return _mac_str; }

VirtioNet::VirtioNet(PCI_Device* d)
  : Virtio(d)
{

  
  printf("\n>>> VirtioNet driver initializing \n");
  

  // Getting the MAC + status 
  get_config(&_conf,sizeof(config));
  
  //Store human readable version
  sprintf(_mac_str,"%1x:%1x:%1x:%1x:%1x:%1x",
          _conf.mac[0],_conf.mac[1], _conf.mac[2],
          _conf.mac[3],_conf.mac[4],_conf.mac[5]);

  
  printf((uint32_t)_conf.mac > 0 ? "\t [*] Mac address: %s \n" :
         "\t [ ] No mac address? : \n",_mac_str);

             //printf("\t VIRTIO Status: 0x%x \n",_conf.status);    
  

  /** Init queues. @todo MOVE */
  int index=0;
  outpw(iobase() + VIRTIO_PCI_QUEUE_SEL, index);
  uint16_t size = inpw(iobase() + VIRTIO_PCI_QUEUE_SIZE);
  if (!size || inpd(iobase() + VIRTIO_PCI_QUEUE_PFN)) 
    printf("Queue %i doesn't exist, or is size 0",index);
  
  printf("\t [*] Virtio Q %i needs %i pointers (%li bytes) \n",
         index, size, size * sizeof(void*));
  
  
  index=1;
  outpw(iobase() + VIRTIO_PCI_QUEUE_SEL, index);
  size = inpw(iobase() + VIRTIO_PCI_QUEUE_SIZE);
  if (!size || inpd(iobase() + VIRTIO_PCI_QUEUE_PFN)) 
    printf("Queue %i doesn't exist, or is size 0",index);
  
  printf("\t [*] Virtio Q %i needs %i pointers (%li bytes) \n",
         index, size, size * sizeof(void*));

  sig_driver_found();
  
  
  printf("\t [*] Signalled driver found \n");

  //__asm__("int $0x36");
  


  auto del=delegate::from_method<VirtioNet,&VirtioNet::irq_handler>(this);  
  IRQ_handler::subscribe(irq(),del);
  IRQ_handler::enable_irq(irq());  
  //IRQ_handler::subscribe(1,del);
  //IRQ_handler::enable_irq(1);
  printf("\t [%s] Link up \n",_conf.status & 1 ? "*":" ");
  // Done
  printf("\n >> Driver initialization complete. \n\n");
    
};  

void VirtioNet::irq_handler(){
  printf("VirtioNet IRQ Handler! \n");
  
  printf("Old status: 0x%x \n",_conf.status);
    
  // Getting the MAC + status 
  get_config(&_conf,sizeof(config));
  printf("New status: 0x%x \n",_conf.status);
  
  
  unsigned char isr = inp(iobase() + VIRTIO_PCI_ISR);
  printf("Virtio ISR: 0x%i \n",isr);
  isr = inp(iobase() + VIRTIO_PCI_ISR);
  printf("Virtio ISR: 0x%i \n",isr);


  eoi(irq());
  
}
