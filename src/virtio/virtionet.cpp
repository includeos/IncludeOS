// Part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and  Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#define PRINT_INFO
#define DEBUG // Allow debuging 
//#define DEBUG2

#include <virtio/virtionet.hpp>
#include <net/packet.hpp>

#include <kernel/irq_manager.hpp>
#include <kernel/syscalls.hpp>
#include <stdio.h>
#include <malloc.h>
#include <string.h>

using namespace net;

const char* VirtioNet::name(){ return "VirtioNet Driver"; }
const net::Ethernet::addr& VirtioNet::mac(){ return _conf.mac; }  

void VirtioNet::get_config(){
  Virtio::get_config(&_conf,_config_length);
};

int drop(std::shared_ptr<Packet> UNUSED(pckt)){
  debug("<VirtioNet->link-layer> No delegate. DROP!\n");
  return -1;
}

VirtioNet::VirtioNet(PCI_Device& d)
  : Virtio(d),
    /** RX que is 0, TX Queue is 1 - Virtio Std. §5.1.2  */
    rx_q(queue_size(0),0,iobase()),  tx_q(queue_size(1),1,iobase()), 
    ctrl_q(queue_size(2),2,iobase()),
    _link_out(net::upstream(drop))
{
  
  INFO("VirtioNet", "Driver initializing");
  
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
  
  
  CHECK ((features() & needed_features) == needed_features,
	 "Negotiated needed features");
  
  CHECK ((features() & wanted_features) == wanted_features,
	 "Negotiated wanted features");

  CHECK(features() & (1 << VIRTIO_NET_F_CSUM),
	"Device handles packets w. partial checksum");

  CHECK(features() & (1 << VIRTIO_NET_F_GUEST_CSUM),
	"Guest handles packets w. partial checksum");

  CHECK(features() & (1 << VIRTIO_NET_F_CTRL_VQ),
       "There's a control queue");

  CHECK(features() & (1 << VIRTIO_F_ANY_LAYOUT), 
       "Queue can handle any header/data layout");
  
  CHECK(features() & (1 << VIRTIO_F_RING_INDIRECT_DESC),
	"We can use indirect descriptors");
  
  CHECK(features() & (1 << VIRTIO_F_RING_EVENT_IDX),
	"There's a Ring Event Index to use");

  CHECK(features() & (1 << VIRTIO_NET_F_MQ),
	"There are multiple queue pairs")
     
  if (features() & (1 << VIRTIO_NET_F_MQ))
    printf("\t\t* max_virtqueue_pairs: 0x%x \n",_conf.max_virtq_pairs);
    
  CHECK(features() & (1 << VIRTIO_NET_F_MRG_RXBUF),
	"Merge RX buffers");
  
   
  // Step 1 - Initialize RX/TX queues
  auto success = assign_queue(0, (uint32_t)rx_q.queue_desc());
  CHECK(success, "RX queue assigned (0x%x) to device",
	(uint32_t)rx_q.queue_desc());
  
  success = assign_queue(1, (uint32_t)tx_q.queue_desc()); 
  CHECK(success, "TX queue assigned (0x%x) to device",
	(uint32_t)tx_q.queue_desc());
  
  // Step 2 - Initialize Ctrl-queue if it exists
  if (features() & (1 << VIRTIO_NET_F_CTRL_VQ)) {
    success = assign_queue(2, (uint32_t)tx_q.queue_desc());
    CHECK(success, "CTRL queue assigned (0x%x) to device",
	  (uint32_t)ctrl_q.queue_desc());
  }
  
  // Step 3 - Fill receive queue with buffers
  // DEBUG: Disable
  INFO("VirtioNet", "Adding %i receive buffers of size %i",
       rx_q.size() / 2, MTUSIZE+sizeof(virtio_net_hdr));
  
  for (int i = 0; i < rx_q.size() / 2; i++) add_receive_buffer();
  
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
  
  CHECK(_conf.mac.major > 0, "Valid Mac address: %s", 
	_conf.mac.str().c_str());

 
  // Step 7 - 9 - GSO: @todo Not using GSO features yet. 

  // Signal setup complete. 
  setup_complete((features() & needed_features) == needed_features);
  CHECK((features() & needed_features) == needed_features, "Signalled driver OK");
  
  // Hook up IRQ handler
  auto del(delegate<void()>::from<VirtioNet,&VirtioNet::irq_handler>(this));
  IRQ_manager::subscribe(irq(),del);
  IRQ_manager::enable_irq(irq());  
    
  // Done
  INFO("VirtioNet", "Driver initialization complete");
  CHECK(_conf.status & 1, "Link up\n");    
  rx_q.kick();
  

};  

/** Port-ish from SanOS */
int VirtioNet::add_receive_buffer(){
  virtio_net_hdr* hdr;
  scatterlist sg[2];  
  
  // Virtio Std. § 5.1.6.3
  auto buf = bufstore_.get_raw_buffer();
  
  debug2("<VirtioNet> Added receive-bufer @ 0x%lx \n", (uint32_t)buf);
  
  hdr = (virtio_net_hdr*)buf;
  
  sg[0].data = hdr;
  
  //NOTE: using separate empty header doesn't work for RX, but it works for TX...
  //sg[0].data = (void*)&empty_header; 
  sg[0].size = sizeof(virtio_net_hdr);
  sg[1].data = buf + sizeof(virtio_net_hdr);
  sg[1].size = MTUSIZE; 
  rx_q.enqueue(sg, 0, 2,buf);  
  
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
  IRQ_manager::eoi(irq());    
  
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
  // A zipper, alternating between sending and receiving
  while(rx_q.new_incoming() or tx_q.new_incoming()){
    
    // Do one RX-packet
    if (rx_q.new_incoming() ){
      data = rx_q.dequeue(&len); //BUG # 102? + sizeof(virtio_net_hdr);
      
      auto pckt_ptr = std::make_shared<Packet>
        (data+sizeof(virtio_net_hdr), // Offset buffer (bufstore knows the offset)
	 MTU()-sizeof(virtio_net_hdr), // Capacity
	 len - sizeof(virtio_net_hdr), release_buffer); // Size
      
      _link_out(pckt_ptr); 
    
      // Requeue a new buffer 
      add_receive_buffer();
      
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

int VirtioNet::transmit(net::Packet_ptr pckt){
  debug2("<VirtioNet> Enqueuing %lib of data. \n",pckt->len());


  /** @note We have to send a virtio header first, then the packet.
      
      From Virtio std. §5.1.6.6: 
      "When using legacy interfaces, transitional drivers which have not 
      negotiated VIRTIO_F_ANY_LAYOUT MUST use a single descriptor for the struct
      virtio_net_hdr on both transmit and receive, with the network data in the 
      following descriptors." 
      
      VirtualBox *does not* accept ANY_LAYOUT, while Qemu does, so this is to 
      support VirtualBox 
  */

  // A scatterlist for virtio-header + data
  scatterlist sg[2];  
  
  // This setup requires all tokens to be pre-chained like in SanOS
  sg[0].data = (void*)&empty_header;
  sg[0].size = sizeof(virtio_net_hdr);
  sg[1].data = (void*)pckt->buffer();
  sg[1].size = pckt->size();
  
  // Enqueue scatterlist, 2 pieces readable, 0 writable.
  tx_q.enqueue(sg, 2, 0, 0);
  
  tx_q.kick();
  
  return 0;
}
