#include <virtio/class_virtionet.hpp>
#include <virtio/virtio.h>
#include <class_irq_handler.hpp>
#include <stdio.h>
#include <syscalls.hpp>
#include <malloc.h>
#include <string.h>

const char* VirtioNet::name(){ return "VirtioNet Driver"; }
const mac_t& VirtioNet::mac(){ return _conf.mac; }  
const char* VirtioNet::mac_str(){ return _mac_str; }

void VirtioNet::get_config(){
  Virtio::get_config(&_conf,_config_length);
};

int drop(uint8_t* UNUSED(data), int len){
  printf("<VirtioNet> Dropping %ib link-layer output. No delegate\n",len);
  return -1;
}

VirtioNet::VirtioNet(PCI_Device* d)
  : Virtio(d),     
    /** RX que is 0, TX Queue is 1 - Virtio Std. §5.1.2  */
    rx_q(queue_size(0),0,iobase()),  tx_q(queue_size(1),1,iobase()), 
    ctrl_q(queue_size(2),2,iobase()),
    _link_out(delegate<int(uint8_t*,int)>(drop))
{
  
  printf("\n>>> VirtioNet driver initializing \n");
  
  uint32_t needed_features = 0 
    | (1 << VIRTIO_NET_F_MAC)
    | (1 << VIRTIO_NET_F_STATUS)
    | (1 << VIRTIO_NET_F_MRG_RXBUF); //Merge RX Buffers (Everything i 1 buffer)
  uint32_t wanted_features = needed_features; /*; 
    | (1 << VIRTIO_NET_F_CSUM);
    | (1 << VIRTIO_NET_F_CTRL_VQ)
    | (1 << VIRTIO_NET_F_GUEST_ANNOUNCE)
    | (1 << VIRTIO_NET_F_CTRL_MAC_ADDR);*/
  
  negotiate_features(wanted_features);
  
  
  printf("\t [%s] Netotiated needed features \n",
         (features() & needed_features) == needed_features ? "x" : " " );
  
  printf("\t [%s] Netotiated wanted features \n",
         (features() & wanted_features) == wanted_features ? "x" : " " );

  printf("\t [%s] Device handles packets w. partial checksum \n",
         features() & (1 << VIRTIO_NET_F_CSUM) ? "x" : "0" );

  printf("\t [%s] Guest handles packets w. partial checksum \n",
         features() & (1 << VIRTIO_NET_F_GUEST_CSUM) ? "x" : "0" );

  printf("\t [%s] There's a control queue \n",
         features() & (1 << VIRTIO_NET_F_CTRL_VQ) ? "x" : "0" );

  printf("\t [%s] There are multiple queue pairs \n",
         features() & (1 << VIRTIO_NET_F_MQ) ? "x" : "0" );

  printf("\t [%s] We can use indirect descriptors \n",
         features() & (1 << VIRTIO_F_RING_INDIRECT_DESC) ? "x" : "0" );
  
  printf("\t [%s] There's a Ring Event Index to use \n",
         features() & (1 << VIRTIO_F_RING_EVENT_IDX) ? "x" : "0" );
     
  if (features() & (1 << VIRTIO_NET_F_MQ))
    printf("\t      max_virtqueue_pairs: 0x%x \n",_conf.max_virtq_pairs);  
  

  printf("\t [%s] Merge RX buffers  \n",
         features() & (1 << VIRTIO_NET_F_MRG_RXBUF) ? "x" : "0" );
  
   
  // Step 1 - Initialize RX/TX queues
  printf("\t [%s] RX queue assigned (0x%lx) to device \n ",
         assign_queue(0, (uint32_t)rx_q.queue_desc()) ? "x":" ",
         (uint32_t)rx_q.queue_desc());
  
  printf("\t [%s] TX queue assigned (0x%lx) to device \n ",
         assign_queue(1, (uint32_t)tx_q.queue_desc()) ? "x":" ",
         (uint32_t)tx_q.queue_desc());

  
  // Step 2 - Initialize Ctrl-queue if it exists
  if (features() & (1 << VIRTIO_NET_F_CTRL_VQ))
    printf("\t [%s] CTRL queue assigned (0x%lx) to device \n ",
           assign_queue(2, (uint32_t)tx_q.queue_desc()) ? "x":" ",
           (uint32_t)ctrl_q.queue_desc());
  
  // Step 3 - Fill receive queue with buffers
  // DEBUG: Disable
  printf("Adding %i receive buffers \n", rx_q.size() / 2);
  for (int i = 0; i < rx_q.size() / 2; i++) add_receive_buffer();
  //add_receive_buffer();

  
  
  // Step 4 - If there are many queues, we should negotiate the number.
  // Set config length, based on whether there are multiple queues
  if (features() & (1 << VIRTIO_NET_F_MQ))
    _config_length = sizeof(config);  
  else
    _config_length = sizeof(config) - sizeof(uint16_t);
  // @todo: Specify how many queues we're going to use.
  
  // Step 5 - get the mac address (we're demanding this feature)
  // Step 6 - get the status - demanding this as well.
  // Getting the MAC + status 
  get_config(); 
  sprintf(_mac_str,"%1x:%1x:%1x:%1x:%1x:%1x", 
          _conf.mac[0],_conf.mac[1], _conf.mac[2],
          _conf.mac[3],_conf.mac[4],_conf.mac[5]);
 
  printf((uint32_t)_conf.mac > 0 ? "\t [*] Mac address: %s \n" :
         "\t [ ] No mac address? : \n",_mac_str);

 
  // Step 7 - 9 - GSO: @todo Not using GSO features yet. 

  // Signal setup complete. 
  
  // DEBUG - disable this
  setup_complete((features() & needed_features) == needed_features);

  printf("\t [%s] Signalled driver OK \n",
         (features() & needed_features) == needed_features ? "*": " ");
  
  
  // Hook up IRQ handler
  //auto del=delegate::from_method<VirtioNet,&VirtioNet::irq_handler>(this);  
  auto del(delegate<void()>::from<VirtioNet,&VirtioNet::irq_handler>(this));
  IRQ_handler::subscribe(irq(),del);
  IRQ_handler::enable_irq(irq());  
  
   
  // Assign Link-layer output to RX Queue
  rx_q.set_data_handler(_link_out);
  
  printf("\t [%s] Link up \n",_conf.status & 1 ? "*":" ");
    
  rx_q.kick();
  
  // Done
  printf("\n >> Driver initialization complete. \n\n");  

  // Test stransmission
  /*
  uint8_t buf[100] = {0};
  memset(buf,0,100);
  Ethernet::header* hdr = (Ethernet::header*)buf;
  hdr->src = {0x8,0x0,0x27,0x9d,0x86,0xe8};
  hdr->dest = {0x8,0x0,0x27,0xac,0x54,0x90};
  hdr->type = Ethernet::ETH_ARP;
  
  linklayer_in(buf,100);
  //add_send_buffer();
  tx_q.kick();*/
  
};  

/** Port-ish from SanOS */
int VirtioNet::add_receive_buffer(){
  virtio_net_hdr* hdr;
  scatterlist sg[2];  
  
  // Virtio Std. § 5.1.6.3
  uint8_t* buf = (uint8_t*)malloc(MTUSIZE + sizeof(virtio_net_hdr));  
  if(!buf) panic("Couldn't allocate memory for VirtioNet RX buffer");

  strcpy ((char*)buf+sizeof(virtio_net_hdr),"Hello World! \n");
  //printf("Buffer data: %s \n",str);
  
  hdr = (virtio_net_hdr*)buf;
  sg[0].data = hdr;
  sg[0].size = sizeof(struct virtio_net_hdr);
  sg[1].data = buf + sizeof(virtio_net_hdr);
  sg[1].size = MTUSIZE; // + sizeof(virtio_net_hdr);
  rx_q.enqueue(sg, 0, 2,buf);
  
  //printf("Buffer data: %s \n",(char*)sg[1].data);
  
  return 0;
}


void VirtioNet::irq_handler(){


  printf("<VirtioNet> handling IRQ \n");
  
  //Virtio Std. § 4.1.5.5, steps 1-3    
  
  // Step 1. read ISR
  unsigned char isr = inp(iobase() + VIRTIO_PCI_ISR);
  
  // Step 2. A)
  if (isr & 1){
    printf("\t <VirtioNet> Queue activity; checking RX Queue \n");
    rx_q.notify();
    printf("\t <VirtioNet> Queue activity; checking TX Queue \n");
    tx_q.notify();
  }
  
  // Step 2. B)
  if (isr & 2){
    printf("\t <VirtioNet> Configuration change:\n");
    
    // Getting the MAC + status 
    printf("\t             Old status: 0x%x\n",_conf.status);      
    get_config();
    printf("\t             New status: 0x%x \n",_conf.status);
  }
  
  // ISR should now be 0
  //isr = inp(iobase() + VIRTIO_PCI_ISR);

  eoi(irq());
  
}


//TEMP for pretty printing 
extern "C"  char *ether2str(Ethernet::addr *hwaddr, char *s);

int VirtioNet::add_send_buffer()
{ 
  virtio_net_hdr* hdr;
  scatterlist sg[2];  
  
  // Virtio Std. § 5.1.6.3
  uint8_t* buf = (uint8_t*)malloc(MTUSIZE + sizeof(virtio_net_hdr));  
  if(!buf) panic("Couldn't allocate memory for VirtioNet RX buffer");

  memset(buf,0,MTUSIZE+sizeof(virtio_net_hdr));
  
  strcpy ((char*)buf+sizeof(virtio_net_hdr),"Hello World! \n");
  //printf("Buffer data: %s \n",str);
  
  hdr = (virtio_net_hdr*)buf;
  
  
  sg[0].data = hdr;
  sg[0].size = sizeof(struct virtio_net_hdr);
  sg[1].data = buf; //sizeof(virtio_net_hdr);
  sg[1].size = MTUSIZE + sizeof(virtio_net_hdr);
  tx_q.enqueue(sg, 2, 0,buf);
  return 0;
}


int VirtioNet::transmit(uint8_t* data, int len){
  printf("<VirtioNet> Enqueuing %ib of data. \n",len);


  Ethernet::header* ehdr = (Ethernet::header*)data;
  
  char* mac = (char*)"00:00:00:00:00:00";
  printf("DATA: Source: %s \n",  ether2str(&(ehdr->src),mac));
  printf("DATA: Dest: %s \n",  ether2str(&(ehdr->dest),mac));

  
  /** @note We have to send a virtio header first, then the packet.
      
      From Virtio std. §5.1.6.6: 
      "When using legacy interfaces, transitional drivers which have not negotiated VIRTIO_F_ANY_LAYOUT MUST use a single descriptor for the struct virtio_net_hdr on both transmit and receive, with the network data in the following descriptors." */

  // A scatterlist for virtio-header + data
  scatterlist sg[2];  

  
  //Allocate a buffer
  int BUFSIZE = sizeof(virtio_net_hdr)+MTUSIZE; //+len
  uint8_t* buf = (uint8_t*)malloc(BUFSIZE);
  
  
  memset(buf,0,BUFSIZE);
  
  //The header (unused for now)
  //virtio_net_hdr* hdr = (virtio_net_hdr*)buf;
  
  
  //UGLY copy data to buffer
  memcpy((char*)buf+sizeof(virtio_net_hdr),data, len);
  
  
  ehdr = (Ethernet::header*)((char*)buf+sizeof(virtio_net_hdr));

  printf("BUF: Source: %s \n",  ether2str(&(ehdr->src),mac));
  printf("BUF: Dest: %s \n",  ether2str(&(ehdr->dest),mac));
  
  //*((uint32_t*)buf+(MTUSIZE -4))=0xac2f54c4;
  
  sg[0].data = buf;
  sg[0].size = sizeof(virtio_net_hdr);
  sg[1].data = buf; // + sizeof(virtio_net_hdr);
  sg[1].size = len;//MTUSIZE + sizeof(virtio_net_hdr);

  //sg[1].data = (void*)data;
  //sg[1].size = len; // + sizeof(virtio_net_hdr);
  
  // Enqueue scatterlist, 2 pieces readable, 0 writable.
  tx_q.enqueue(sg, 2, 0, 0);
  
  tx_q.kick();
  
  return 0;
}
