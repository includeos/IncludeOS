#define DEBUG
#define DEBUG2
#include <virtio/block.hpp>

#include <kernel/irq_manager.hpp>
#include <hw/pci.hpp>
#include <cassert>
#include <stdlib.h>

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

#define FEAT(x)  (1 << x)

VirtioBlk::VirtioBlk(hw::PCI_Device& d)
: Virtio(d),
  req(queue_size(0), 0, iobase()),
                       request_counter(0)
{
  INFO("VirtioBlk", "Driver initializing");
  
  uint32_t needed_features =
    FEAT(VIRTIO_BLK_F_BLK_SIZE);
  negotiate_features(needed_features);
  
  CHECK(features() & FEAT(VIRTIO_BLK_F_BARRIER),
        "Barrier is enabled");
  CHECK(features() & FEAT(VIRTIO_BLK_F_SIZE_MAX),
        "Size-max is known");
  CHECK(features() & FEAT(VIRTIO_BLK_F_SEG_MAX),
        "Seg-max is known");
  CHECK(features() & FEAT(VIRTIO_BLK_F_GEOMETRY),
        "Geometry structure is used");
  CHECK(features() & FEAT(VIRTIO_BLK_F_RO),
        "Device is read-only");
  CHECK(features() & FEAT(VIRTIO_BLK_F_BLK_SIZE),
        "Block-size is known");
  CHECK(features() & FEAT(VIRTIO_BLK_F_SCSI),
        "SCSI is enabled :(");
  CHECK(features() & FEAT(VIRTIO_BLK_F_FLUSH),
        "Flush enabled");
  
  
  CHECK ((features() & needed_features) == needed_features,
         "Negotiated needed features");
  
  // Step 1 - Initialize REQ queue
  auto success = assign_queue(0, (uint32_t) req.queue_desc());
  CHECK(success, "Request queue assigned (0x%x) to device",
        (uint32_t) req.queue_desc());
  
  // Step 3 - Fill receive queue with buffers
  // DEBUG: Disable
  INFO("VirtioBlk", "Queue size: %i\tRequest size: %u\n",
       req.size(), sizeof(request_t));
  
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
  INFO("VirtioBlk", "Block device with %llu sectors capacity",
       config.capacity);
  //CHECK(config.status == VIRTIO_BLK_S_OK, "Link up\n");    
  req.kick();
}

void VirtioBlk::get_config()
{
  Virtio::get_config(&config, sizeof(virtio_blk_config_t));
}

void VirtioBlk::irq_handler()
{
  debug2("<VirtioBlk> IRQ handler\n");

  //Virtio Std. § 4.1.5.5, steps 1-3    
  
  // Step 1. read ISR
  unsigned char isr = hw::inp(iobase() + VIRTIO_PCI_ISR);
  
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
      //debug("\t             Old status: 0x%x\n", config.status);      
      get_config();
      //debug("\t             New status: 0x%x \n", config.status);
    }
  IRQ_manager::eoi(irq());
}

void VirtioBlk::service_RX()
{
  printf("VirtioBlk interrupt handler\n");
  req.disable_interrupts();
  
  uint32_t received = 0;
  uint32_t len;
  request_t* hdr;
  
  while ((hdr = (request_t*) req.dequeue(len)) != nullptr)
  {
    printf("service_RX() received %u bytes for sector %llu\n", 
          len, hdr->hdr.sector);
    //
    blk_resp_t* resp = &hdr->resp;
    printf("blk response: %u\n", resp->status);
    
    uint8_t* copy = new uint8_t[SECTOR_SIZE];
    memcpy(copy, hdr->io.sector, SECTOR_SIZE);
    auto buf = buffer_t(copy, std::default_delete<uint8_t[]>());
    
    printf("STATUS: [%u]\nCalling handler: %p\n", 
        resp->status, &hdr->io.handler);
    hdr->io.handler(buf);
    
    received++;
  }
  if (received == 0)
  {
    //printf("service_RX() error processing requests\n");
  }
  
  req.enable_interrupts();
}

void VirtioBlk::read (block_t blk, on_read_func func)
{
  // Virtio Std. § 5.1.6.3
  auto* vbr = new request_t;
  
  vbr->hdr.type   = VIRTIO_BLK_T_IN;
  vbr->hdr.ioprio = 0;
  vbr->hdr.sector = blk;
  vbr->io.handler = func;
  vbr->resp.status  = VIRTIO_BLK_S_IOERR;
  
  printf("Enqueue handler: %p, total: %u\n", 
      &vbr->io.handler, sizeof(request_t));
  //
  req.enqueue(&vbr->hdr, sizeof(scsi_header_t), 1, false); // out
  req.enqueue(&vbr->io,  sizeof(blk_io_t),      0, false); // in
  req.enqueue(&vbr->resp, sizeof(blk_resp_t),   0, true);  // in, last
  req.kick();
}

VirtioBlk::buffer_t VirtioBlk::read_sync(block_t)
{
  return buffer_t();
}
