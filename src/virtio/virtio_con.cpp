#define DEBUG
#define DEBUG2
#include <virtio/virtio_con.hpp>

#include <kernel/irq_manager.hpp>
#include <hw/pci.hpp>
#include <cassert>
#include <cstdlib>

#define VIRTIO_CONSOLE_F_SIZE          0
#define VIRTIO_CONSOLE_F_MULTIPORT     1
#define VIRTIO_CONSOLE_F_EMERG_WRITE   2

#define VIRTIO_CONSOLE_DEVICE_READY    0
#define VIRTIO_CONSOLE_DEVICE_ADD      1
#define VIRTIO_CONSOLE_DEVICE_REMOVE   2
#define VIRTIO_CONSOLE_PORT_READY      3
#define VIRTIO_CONSOLE_CONSOLE_PORT    4
#define VIRTIO_CONSOLE_RESIZE          5
#define VIRTIO_CONSOLE_PORT_OPEN       6
#define VIRTIO_CONSOLE_PORT_NAME       7


#define FEAT(x)  (1 << x)

VirtioCon::VirtioCon(PCI_Device& d)
  : Virtio(d),
    rx(queue_size(0), 0, iobase()),
    tx(queue_size(1), 1, iobase()),
    ctl_rx(queue_size(2), 2, iobase()),
    ctl_tx(queue_size(3), 3, iobase())
{
  INFO("VirtioCon", "Driver initializing");
  
  uint32_t needed_features =
      FEAT(VIRTIO_CONSOLE_F_SIZE);
  negotiate_features(needed_features);
  
  CHECK(features() & FEAT(VIRTIO_CONSOLE_F_SIZE),
    "Barrier is enabled");
  CHECK(features() & FEAT(VIRTIO_CONSOLE_F_MULTIPORT),
    "Console dimensions is known");
  CHECK(features() & FEAT(VIRTIO_CONSOLE_F_EMERG_WRITE),
    "Seg-max is known");
  
  CHECK ((features() & needed_features) == needed_features,
    "Negotiated needed features");
  
  // Step 1 - Initialize receive queues
  auto success = assign_queue(0, (uint32_t) rx.queue_desc());
  CHECK(success, "Receive queue assigned (0x%x) to device",
    (uint32_t) rx.queue_desc());
  
  success = assign_queue(2, (uint32_t) rx.queue_desc());
  CHECK(success, "Control rx queue assigned (0x%x) to device",
    (uint32_t) ctl_rx.queue_desc());
  
  // Step 3 - Fill receive queue with buffers
  // DEBUG: Disable
  INFO("VirtioCon", "Queue size   rx: %d  tx: %d\n", 
        rx.size(), tx.size());
  
  // Get device configuration
  get_config();
  
  // Signal setup complete. 
  setup_complete((features() & needed_features) == needed_features);
  CHECK((features() & needed_features) == needed_features, "Signalled driver OK");
  
  // Hook up IRQ handler (inherited from Virtio)
  auto del(delegate<void()>::from<VirtioCon, &VirtioCon::irq_handler>(this));
  IRQ_manager::subscribe(irq(),del);
  IRQ_manager::enable_irq(irq());  
  
  // Done
  INFO("VirtioCon", "Console with size (%u, %u), %u ports",
      config.cols, config.rows, config.max_nr_ports);
  //CHECK(config.status == VIRTIO_BLK_S_OK, "Link up\n");    
  rx.kick();
}

void VirtioCon::get_config()
{
  Virtio::get_config(&config, sizeof(console_config));
}

void VirtioCon::irq_handler()
{
  debug2("<VirtioCon> IRQ handler\n");

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
    debug("\t <VirtioCon> Configuration change:\n");
    
    // Getting the MAC + status 
    //debug("\t             Old status: 0x%x\n", config.status);      
    get_config();
    //debug("\t             New status: 0x%x \n", config.status);
  }
  IRQ_manager::eoi(irq());
}

void VirtioCon::service_RX()
{
  rx.disable_interrupts();
  
  uint32_t received = 0;
  uint32_t len;
  char*    condata;
  while ((condata = (char*) rx.dequeue(&len)) != nullptr)
  {
    printf("service_RX() received %u bytes from virtio console\n", len);
    printf("Data: %s\n", condata);
    //vbr->handler(0, vbr->sector);
    received++;
  }
  if (received == 0)
  {
    printf("service_RX() error processing requests\n");
  }
  
  rx.enable_interrupts();
}

void VirtioCon::write (
      const void* data, 
      size_t len, 
      on_write_func callback)
{
  printf("Writing %u bytes to console\n", len);
  
  scatterlist sg[1];
  sg[0].data = (void*) data;
  sg[0].size = len;   // +1?
  
  tx.enqueue(sg, 0, 1, (void*) data);
  tx.kick();
}
