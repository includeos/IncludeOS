#define DEBUG
#define DEBUG2
#include <virtio/console.hpp>

#include <kernel/irq_manager.hpp>
#include <hw/pci.hpp>
#include <cassert>
#include <cstring>
extern "C"
{ void panic(const char*); }

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

VirtioCon::VirtioCon(hw::PCI_Device& d)
: Virtio(d),
                       rx(queue_size(0), 0, iobase()),
                       tx(queue_size(1), 1, iobase()),
                       ctl_rx(queue_size(2), 2, iobase()),
  ctl_tx(queue_size(3), 3, iobase())
{
  INFO("VirtioCon", "Driver initializing");

  uint32_t needed_features =
    FEAT(VIRTIO_CONSOLE_F_MULTIPORT);
  negotiate_features(needed_features);

  CHECK(features() & FEAT(VIRTIO_CONSOLE_F_SIZE),
        "Valid console dimensions");
  CHECK(features() & FEAT(VIRTIO_CONSOLE_F_MULTIPORT),
        "Multiple ports support");
  CHECK(features() & FEAT(VIRTIO_CONSOLE_F_EMERG_WRITE),
        "Emergency write support");

  CHECK ((features() & needed_features) == needed_features,
         "Negotiated needed features");

  // Step 1 - Initialize queues
  auto success = assign_queue(0, (uint32_t) rx.queue_desc());
  CHECK(success, "Receive queue assigned (0x%x) to device",
        (uint32_t) rx.queue_desc());

  success = assign_queue(1, (uint32_t) tx.queue_desc());
  CHECK(success, "Transmit queue assigned (0x%x) to device",
        (uint32_t) tx.queue_desc());

  success = assign_queue(2, (uint32_t) ctl_rx.queue_desc());
  CHECK(success, "Control rx queue assigned (0x%x) to device",
        (uint32_t) ctl_rx.queue_desc());

  success = assign_queue(3, (uint32_t) ctl_tx.queue_desc());
  CHECK(success, "Control tx queue assigned (0x%x) to device",
        (uint32_t) ctl_tx.queue_desc());

  /*
    success = assign_queue(4, (uint32_t) rx1.queue_desc());
    CHECK(success, "rx1 queue assigned (0x%x) to device",
    (uint32_t) rx1.queue_desc());

    success = assign_queue(5, (uint32_t) tx1.queue_desc());
    CHECK(success, "tx1 queue assigned (0x%x) to device",
    (uint32_t) tx1.queue_desc());
  */

  // Step 3 - Fill receive queue with buffers
  INFO("VirtioCon", "Queue size   rx: %d  tx: %d\n",
       rx.size(), tx.size());

  // Get device configuration
  get_config();

  // Signal setup complete.
  setup_complete((features() & needed_features) == needed_features);
  CHECK((features() & needed_features) == needed_features, "Signalled driver OK");

  // Hook up IRQ handler (inherited from Virtio)
  auto del(delegate<void()>::from<VirtioCon, &VirtioCon::irq_handler>(this));
  IRQ_manager::subscribe(irq(), del);
  IRQ_manager::enable_irq(irq());

  // Done
  INFO("VirtioCon", "Console with size (%u, %u), %u ports",
       config.cols, config.rows, config.max_nr_ports);
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
      debug("\t <VirtioCon> Configuration change:\n");

      //debug("\t             Old status: 0x%x\n", config.status);
      get_config();
      //debug("\t             New status: 0x%x \n", config.status);
    }
  IRQ_manager::eoi(irq());
}

void VirtioCon::service_RX()
{
  rx.disable_interrupts();

  while (rx.new_incoming())
    {
      uint32_t len = 0;
      char* condata = (char*) rx.dequeue(&len);

      uint32_t dontcare;
      rx.dequeue(&dontcare);

      if (condata)
        {
          //printf("service_RX() received %u bytes from virtio console\n", len);
          //printf("Data: %s\n", condata);
          //vbr->handler(0, vbr->sector);
        }
      else
        {
          // acknowledgement
          //printf("No data, just len = %d\n", len);
        }
    }

  rx.enable_interrupts();
}

void VirtioCon::write (
                       const void* data,
                       size_t len)
{
  char* heapdata = new char[len];
  memcpy(heapdata, data, len);

  scatterlist sg[1];
  sg[0].data = (void*) heapdata;
  sg[0].size = len;   // +1?

  tx.enqueue(sg, 1, 0, (void*) data);
  tx.kick();
}
