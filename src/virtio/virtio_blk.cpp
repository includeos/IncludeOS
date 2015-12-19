#include <virtio/virtio_blk.hpp>

#include <kernel/irq_manager.hpp>

#define VIRTIO_BLK_F_BARRIER   0
#define VIRTIO_BLK_F_SIZE_MAX  1
#define VIRTIO_BLK_F_SEG_MAX   2
#define VIRTIO_BLK_F_GEOMETRY  4
#define VIRTIO_BLK_F_RO        5
#define VIRTIO_BLK_F_BLK_SIZE  6
#define VIRTIO_BLK_F_SCSI      7
#define VIRTIO_BLK_F_FLUSH     9

#define VIRTIO_BLK_T_IN         0
#define VIRTIO_BLK_T_OUT        1
#define VIRTIO_BLK_T_FLUSH      4
#define VIRTIO_BLK_T_FLUSH_OUT  5
#define VIRTIO_BLK_T_BARRIER    0x80000000

#define VIRTIO_BLK_S_OK        0
#define VIRTIO_BLK_S_IOERR     1
#define VIRTIO_BLK_S_UNSUPP    2

void drop_disk_event(const char*) {}

VirtioBlk::VirtioBlk(PCI_Device& d)
  : Virtio(d),
    req(queue_size(0),0, iobase())
{
  INFO("VirtioBlk", "Driver initializing");
  
  uint32_t needed_features = 0 
  //  | (1 << VIRTIO_BLK_F_SIZE_MAX)
  //  | (1 << VIRTIO_BLK_F_SEG_MAX)
    | (1 << VIRTIO_BLK_F_RO);
  negotiate_features(needed_features);
  
  
  CHECK ((features() & needed_features) == needed_features,
    "Negotiated needed features");
  
  CHECK(features() & (1 << VIRTIO_BLK_F_RO),
    "Device is read-only");
  
  // Step 1 - Initialize RX/TX queues
  auto success = assign_queue(0, (uint32_t) req.queue_desc());
  CHECK(success, "Request queue assigned (0x%x) to device",
    (uint32_t) req.queue_desc());
  
  for (int i = 0; i < req.size() / 2; i++)
    add_receive_buffer();
  
  // Get device configuration
  get_config();  
  
  // Signal setup complete. 
  setup_complete((features() & needed_features) == needed_features);
  CHECK((features() & needed_features) == needed_features, "Signalled driver OK");
  
  // Hook up IRQ handler (inherited from Virtio)
  auto del(delegate<void()>::from<VirtioBlk, &VirtioBlk::irq_handler>(this));
  IRQ_manager::subscribe(irq(),del);
  IRQ_manager::enable_irq(irq());  
  
  // Done
  INFO("VirtioBlk", "Driver initialization complete");
  //CHECK(config.status == VIRTIO_BLK_S_OK, "Link up\n");    
  req.kick();
}

int VirtioBlk::add_receive_buffer()
{
  scatterlist sg[2];  
  const block_t total = block_size() + sizeof(virtio_blk_request_t);
  
  // Virtio Std. § 5.1.6.3
  //auto buf = bufstore_.get_raw_buffer();
  char* buf = new char[total];
  
  virtio_blk_request_t* vbr = (virtio_blk_request_t*) buf;
  sg[0].data = vbr;
  
  //NOTE: using separate empty header doesn't work for RX, but it works for TX...
  //sg[0].data = (void*)&empty_header; 
  sg[0].size = sizeof(virtio_blk_request_t);
  sg[1].data = buf + sizeof(virtio_blk_request_t);
  sg[1].size = total; 
  req.enqueue(sg, 0, 2,buf);  
  return 0;
}

void VirtioBlk::get_config()
{
  Virtio::get_config(&config, sizeof(virtio_blk_config_t));
}

void VirtioBlk::irq_handler()
{
  debug2("<VirtioNet> handling IRQ \n");

  //Virtio Std. § 4.1.5.5, steps 1-3    
  
  // Step 1. read ISR
  unsigned char isr = inp(iobase() + VIRTIO_PCI_ISR);
  
  // Step 2. A) - one of the queues have changed
  if (isr & 1)
  {
    // This now means service RX & TX interchangeably
    service_RX();
  }
  
  // Step 2. B)
  if (isr & 2)
  {
    debug("\t <VirtioBlk> Configuration change:\n");
    
    // Getting the MAC + status 
    debug("\t             Old status: 0x%x\n",_conf.status);      
    get_config();
    debug("\t             New status: 0x%x \n",_conf.status);
  }
  IRQ_manager::eoi(irq());
}

void VirtioBlk::service_RX()
{
  printf("Received some data from VirtioBlk device\n");
  printf("I'm going to lie down and pretend I'm dead now\n");
}

void VirtioBlk::read (block_t blk, on_read_func func)
{
  printf("Sending read request for block %llu\n", blk);
}
