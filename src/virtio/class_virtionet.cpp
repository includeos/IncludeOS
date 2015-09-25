//#define DEBUG // Allow debuging 
//#define DEBUG2

#include <virtio/class_virtionet.hpp>
//#include <virtio/virtio.h>
#include <class_irq_handler.hpp>
#include <stdio.h>
#include <syscalls.hpp>
#include <malloc.h>
#include <string.h>

using namespace net;

const char* VirtioNet::name(){ return "VirtioNet Driver"; }
const net::Ethernet::addr& VirtioNet::mac(){ return _conf.mac; }  
const char* VirtioNet::mac_str(){ return _mac_str; }

void VirtioNet::get_config(){
  Virtio::get_config(&_conf,_config_length);
};

int drop(std::shared_ptr<Packet> UNUSED(pckt)){
  debug("<VirtioNet->link-layer> No delegate. DROP!\n");
  return -1;
}

VirtioNet::VirtioNet(PCI_Device* d)
  : Virtio(d),     
    /** RX que is 0, TX Queue is 1 - Virtio Std. §5.1.2  */
    rx_q(queue_size(0),0,iobase()),  tx_q(queue_size(1),1,iobase()), 
    ctrl_q(queue_size(2),2,iobase()),
    _link_out(net::upstream(drop))
{
  
  printf("\n>>> VirtioNet driver initializing \n");
  
  uint32_t needed_features = 0 
    | (1 << VIRTIO_NET_F_MAC)
    | (1 << VIRTIO_NET_F_STATUS);
  //| (1 << VIRTIO_NET_F_MRG_RXBUF); //Merge RX Buffers (Everything i 1 buffer)
  uint32_t wanted_features = needed_features; /*; 
    | (1 << VIRTIO_NET_F_CSUM)
    | (1 << VIRTIO_F_ANY_LAYOUT)
    | (1 << VIRTIO_NET_F_CTRL_VQ)
    | (1 << VIRTIO_NET_F_GUEST_ANNOUNCE)
    | (1 << VIRTIO_NET_F_CTRL_MAC_ADDR);*/
  
  negotiate_features(wanted_features);
  
  
  printf("\t [%s] Negotiated needed features \n",
         (features() & needed_features) == needed_features ? "x" : " " );
  
  printf("\t [%s] Negotiated wanted features \n",
         (features() & wanted_features) == wanted_features ? "x" : " " );

  printf("\t [%s] Device handles packets w. partial checksum \n",
         features() & (1 << VIRTIO_NET_F_CSUM) ? "x" : "0" );

  printf("\t [%s] Guest handles packets w. partial checksum \n",
         features() & (1 << VIRTIO_NET_F_GUEST_CSUM) ? "x" : "0" );

  printf("\t [%s] There's a control queue \n",
         features() & (1 << VIRTIO_NET_F_CTRL_VQ) ? "x" : "0" );

  printf("\t [%s] Queue can handle any header/data layout \n",
         features() & (1 << VIRTIO_F_ANY_LAYOUT) ? "x" : "0" );

  printf("\t [%s] We can use indirect descriptors \n",
         features() & (1 << VIRTIO_F_RING_INDIRECT_DESC) ? "x" : "0" );
  
  printf("\t [%s] There's a Ring Event Index to use \n",
         features() & (1 << VIRTIO_F_RING_EVENT_IDX) ? "x" : "0" );

  printf("\t [%s] There are multiple queue pairs \n",
         features() & (1 << VIRTIO_NET_F_MQ) ? "x" : "0" );
     
  if (features() & (1 << VIRTIO_NET_F_MQ))
    printf("\t      max_virtqueue_pairs: 0x%x \n",_conf.max_virtq_pairs);  
  

  printf("\t [%s] Merge RX buffers  \n",
         features() & (1 << VIRTIO_NET_F_MRG_RXBUF) ? "x" : "0" );
  
   
  // Step 1 - Initialize RX/TX queues
  printf("\t [%s] RX queue assigned (0x%lx) to device \n",
         assign_queue(0, (uint32_t)rx_q.queue_desc()) ? "x":" ",
         (uint32_t)rx_q.queue_desc());
  
  printf("\t [%s] TX queue assigned (0x%lx) to device \n",
         assign_queue(1, (uint32_t)tx_q.queue_desc()) ? "x":" ",
         (uint32_t)tx_q.queue_desc());

  
  // Step 2 - Initialize Ctrl-queue if it exists
  if (features() & (1 << VIRTIO_NET_F_CTRL_VQ))
    printf("\t [%s] CTRL queue assigned (0x%lx) to device \n",
           assign_queue(2, (uint32_t)tx_q.queue_desc()) ? "x":" ",
           (uint32_t)ctrl_q.queue_desc());
  
  // Step 3 - Fill receive queue with buffers
  // DEBUG: Disable
  printf(" >> Adding %i receive buffers of size %li \n", 
         rx_q.size() / 2, MTUSIZE+sizeof(virtio_net_hdr));
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
  printf((uint32_t)_conf.mac.major > 0 ? "\t [*] Mac address: %s \n" : 
         "\t [ ] No mac address? : \n",_conf.mac.str().c_str());

 
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
  //rx_q.set_data_handler(_link_out);
  
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
  //uint8_t* buf = (uint8_t*)malloc(MTUSIZE);
  debug2("<VirtioNet> Added receive-bufer @ 0x%lx \n",(uint32_t)buf);
  memset(buf,0,MTUSIZE+sizeof(virtio_net_hdr));
  
  if(!buf) panic("Couldn't allocate memory for VirtioNet RX buffer");
  
  hdr = (virtio_net_hdr*)buf;
  
  //hdr->hdr_len = sizeof(virtio_net_hdr);
  //hdr->num_buffers = 1;
  
  
  sg[0].data = hdr;
  
  //Wow, using separate empty header doesn't work for RX, but it works for TX...
  //sg[0].data = (void*)&empty_header; 
  sg[0].size = sizeof(virtio_net_hdr);
  sg[1].data = buf + sizeof(virtio_net_hdr);
  sg[1].size = MTUSIZE; // + sizeof(virtio_net_hdr);
  rx_q.enqueue(sg, 0, 2,buf);
  
  //printf("Buffer data: %s \n",(char*)sg[1].data);
  
  return 0;
}

int VirtioNet::add_receive_buffer(uint8_t* buf, int len){
  
  scatterlist sg[2];  
  
  // Wow, see above; separate empty header only works for TX in Qemu
  //sg[0].data = (void*)&empty_header;
  sg[0].data = buf;
  sg[0].size = sizeof(virtio_net_hdr);
  sg[1].data = buf + sizeof(virtio_net_hdr);
  sg[1].size = len - sizeof(virtio_net_hdr);
  rx_q.enqueue(sg, 0, 2,0);
  
  debug2("<VirtioNet> Added receive buffer \n");
  
  return 0;
}



void VirtioNet::irq_handler(){

  debug2("<VirtioNet> handling IRQ \n");

  //Virtio Std. § 4.1.5.5, steps 1-3    
  
  // Step 1. read ISR
  unsigned char isr = inp(iobase() + VIRTIO_PCI_ISR);
  
  // Step 2. A) - one of the queues have changed
  if (isr & 1){
    
    // This now means service RX & TX interchangeably
    service_RX();
    
    // We need a zipper-solution; we can't receive n packets before sending 
    // anything - that's unfair.
    
    //service_TX();
  }
  
  // Step 2. B)
  if (isr & 2){
    debug("\t <VirtioNet> Configuration change:\n");
    
    // Getting the MAC + status 
    debug("\t             Old status: 0x%x\n",_conf.status);      
    get_config();
    debug("\t             New status: 0x%x \n",_conf.status);
  }
  IRQ_handler::eoi(irq());    
  
}

void VirtioNet::service_RX(){
  debug2("<RX Queue> %i new packets, %i available tokens \n",
        rx_q.new_incoming(),rx_q.num_avail());
  
  
  /** For RX, we dequeue, add new buffers and let receiver is responsible for 
      memory management (they know when they're done with the packet.) */
  
  int i = 0;
  uint32_t len = 0;
  uint8_t* data;
  
  rx_q.disable_interrupts();
  // We need a zipper
  while(rx_q.new_incoming() or tx_q.new_incoming()){
    
    // Do one RX-packet
    if (rx_q.new_incoming() ){
      data = rx_q.dequeue(&len); //BUG # 102? + sizeof(virtio_net_hdr);
      
      // We're passing a stack-pointer here. That's dangerous if the packet 
      // is supposed to be kept, somewhere up the stack. 
      auto pckt_ptr = std::make_shared<Packet>
        (Packet(data+sizeof(virtio_net_hdr), len - sizeof(virtio_net_hdr), Packet::UPSTREAM));
      
      _link_out(pckt_ptr); 
    
      // Requeue the buffer
      add_receive_buffer(data,MTUSIZE + sizeof(virtio_net_hdr));
      i++;

    }
    debug2("<VirtioNet> Service loop about to kick RX if %i \n",i);

    if (i)
      rx_q.kick();
    
    // Do one TX-packet
    if (tx_q.new_incoming()){
      tx_q.dequeue(&len);      
    }
    
    rx_q.enable_interrupts();
  }
  
  debug2("<VirtioNet> Done servicing queues\n");
}

void VirtioNet::service_TX(){
  debug2("<TX Queue> %i transmitted, %i waiting packets\n",
        tx_q.new_incoming(),tx_q.num_avail());
  
  uint32_t len = 0;
  int i = 0;  
  
  /** For TX, just dequeue all incoming tokens.      
      
      Sender allocated the buffer and is responsible for memory management. 
      @todo Sender doesn't know when the packet is transmitted; deal with it. */
  for (;i < tx_q.new_incoming(); i++)
    tx_q.dequeue(&len);
  
  debug2("\t Dequeued %i packets \n",i);
  // Deallocate buffer. 
}


//TEMP for pretty printing 
extern "C"  char *ether2str(Ethernet::addr *hwaddr, char *s);


constexpr VirtioNet::virtio_net_hdr VirtioNet::empty_header;

int VirtioNet::transmit(std::shared_ptr<net::Packet>& pckt){
  debug2("<VirtioNet> Enqueuing %lib of data. \n",pckt->len());


  /** @note We have to send a virtio header first, then the packet.
      
      From Virtio std. §5.1.6.6: 
      "When using legacy interfaces, transitional drivers which have not 
      negotiated VIRTIO_F_ANY_LAYOUT MUST use a single descriptor for the struct
      virtio_net_hdr on both transmit and receive, with the network data in the 
      following descriptors." 
      
      VirtualBox *does not* accept ANY_LAYOUT, while Qemu does, so this is to 
      support VirtualBox (allthough VirtualBox won't eat my packets anyway)
  */

  // A scatterlist for virtio-header + data
  scatterlist sg[2];  
  
  // This setup requires all tokens to be pre-chained like in SanOS
  sg[0].data = (void*)&empty_header;
  sg[0].size = sizeof(virtio_net_hdr);
  sg[1].data = (void*)pckt->buffer();
  sg[1].size = pckt->len();
  
  // Enqueue scatterlist, 2 pieces readable, 0 writable.
  tx_q.enqueue(sg, 2, 0, 0);
  
  tx_q.kick();
  
  return 0;
}
