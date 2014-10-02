#include <virtio/class_virtionet.hpp>
#include <virtio/virtio.h>
#include <class_irq_handler.hpp>
#include <stdio.h>
#include <syscalls.hpp>

const char* VirtioNet::name(){ return "VirtioNet Driver"; }
const mac_t& VirtioNet::mac(){ return _conf.mac; }  
const char* VirtioNet::mac_str(){ return _mac_str; }

VirtioNet::VirtioNet(PCI_Device* d)
  : Virtio(d),     
    /** RX que is 0, TX Queue is 1 - Virtio Std. §5.1.2  */
    rx_q(queue_size(0)),  tx_q(queue_size(1))
{
  
  printf("\n>>> VirtioNet driver initializing \n");
  
  uint32_t needed_features = 0 
    | (1 << VIRTIO_NET_F_MAC)
    | (1 << VIRTIO_NET_F_STATUS);
  uint32_t wanted_features = needed_features; /*
    | (1 << VIRTIO_NET_F_CTRL_VQ)
    | (1 << VIRTIO_NET_F_GUEST_ANNOUNCE)
    | (1 << VIRTIO_NET_F_CTRL_MAC_ADDR);*/
  
  negotiate_features(wanted_features);
  
  
  printf("\t [%s] Netotiated needed features \n",
         (features() & needed_features) == needed_features ? "x" : " " );
  
  printf("\t [%s] Netotiated wanted features \n",
         (features() & wanted_features) == wanted_features ? "x" : " " );

  printf("\t [%s] Device handles packets w. partial checksum \n",
         features() & VIRTIO_NET_F_CSUM ? "x" : "0" );

  printf("\t [%s] Guest handles packets w. partial checksum \n",
         features() & VIRTIO_NET_F_GUEST_CSUM ? "x" : "0" );
    
  // Getting the MAC + status 
  get_config(&_conf,sizeof(config));
  
  // Store human readable version
  sprintf(_mac_str,"%1x:%1x:%1x:%1x:%1x:%1x",
          _conf.mac[0],_conf.mac[1], _conf.mac[2],
          _conf.mac[3],_conf.mac[4],_conf.mac[5]);

  
  printf((uint32_t)_conf.mac > 0 ? "\t [*] Mac address: %s \n" :
         "\t [ ] No mac address? : \n",_mac_str);

  // Initialize queues  
  printf("\t [%s] RX queue assigned (0x%lx) to device \n ",
         assign_queue(0, (uint32_t)rx_q.queue_desc()) ? "x":" ",
         (uint32_t)rx_q.queue_desc());
         
  printf("\t [%s] TX queue assigned (0x%lx) to device \n ",
         assign_queue(1, (uint32_t)tx_q.queue_desc()) ? "x":" ",
         (uint32_t)tx_q.queue_desc());

  
  // Signal setup complete. 
  setup_complete((features() & needed_features) == needed_features);    
  printf("\t [%s] Signalled driver OK \n",
         (features() & needed_features) == needed_features ? "*": " ");
  
  
  // Hook up IRQ handler
  auto del=delegate::from_method<VirtioNet,&VirtioNet::irq_handler>(this);  
  IRQ_handler::subscribe(irq(),del);
  IRQ_handler::enable_irq(irq());  
  
  printf("\t [%s] Link up \n",_conf.status & 1 ? "*":" ");
  // Done
  printf("\n >> Driver initialization complete. \n\n");
    
};  

void VirtioNet::irq_handler(){


  printf("VirtioNet IRQ Handler! \n");
  
  //Virtio Std. § 4.1.5.5, steps 1-3    
  
  // Step 1. read ISR
  unsigned char isr = inp(iobase() + VIRTIO_PCI_ISR);
  
  // Step 2. A)
  if (isr & 1){
    printf("\t VIRTIO Queue acrtivity. Checking queues \n");
    rx_q.notify();
    tx_q.notify();
  }
  
  // Step 2. B)
  if (isr & 2){
    printf("\t VIRTIO Configuration change: ");
    
    // Getting the MAC + status 
    printf("Old status: 0x%x ",_conf.status);      
    get_config(&_conf,sizeof(config));
    printf("New status: 0x%x \n",_conf.status);
  }
  
  // ISR should now be 0
  //isr = inp(iobase() + VIRTIO_PCI_ISR);

  eoi(irq());
  
}
