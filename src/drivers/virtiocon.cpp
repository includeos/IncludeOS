#include "virtiocon.hpp"
#include <kernel/events.hpp>
#include <hw/pci.hpp>
#include <cassert>
#include <cstring>

#ifdef VCON_DEBUG
#define VCPRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define VCPRINT(fmt, ...) /* fmt */
#endif

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

static int index_counter = 0;
VirtioCon::VirtioCon(hw::PCI_Device& d)
: Virtio(d), m_index(index_counter)
{
  index_counter++;
  INFO("VirtioCon", "Driver initializing");

  new (&rx) Virtio::Queue(device_name() + ".rx_q", queue_size(0), 0, iobase());
  new (&tx) Virtio::Queue(device_name() + ".tx_q", queue_size(1), 1, iobase());
  new (&ctl_rx) Virtio::Queue(device_name() + ".ctl_rx_q", queue_size(2), 2, iobase());
  new (&ctl_tx) Virtio::Queue(device_name() + ".ctl_tx_q", queue_size(3), 3, iobase());

  const uint32_t needed_features =
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
  auto success = assign_queue(0, rx.queue_desc());
  CHECK(success, "Receive queue assigned (%p) to device",
        rx.queue_desc());

  success = assign_queue(1, tx.queue_desc());
  CHECK(success, "Transmit queue assigned (%p) to device",
        tx.queue_desc());

  success = assign_queue(2, ctl_rx.queue_desc());
  CHECK(success, "Control rx queue assigned (%p) to device",
        ctl_rx.queue_desc());

  success = assign_queue(3, ctl_tx.queue_desc());
  CHECK(success, "Control tx queue assigned (%p) to device",
        ctl_tx.queue_desc());

  // Step 3 - Fill receive queue with buffers
  INFO("VirtioCon", "Queue size   rx: %d  tx: %d\n",
       rx.size(), tx.size());

  // Get device configuration
  get_config();

  // Signal setup complete.
  setup_complete((features() & needed_features) == needed_features);
  CHECK((features() & needed_features) == needed_features, "Signalled driver OK");

  if (has_msix())
  {
    assert(get_msix_vectors() >= 3);
    auto& irqs = this->get_irqs();
    Events::get().subscribe(irqs[0], {this, &VirtioCon::msix_recv_handler});
    Events::get().subscribe(irqs[1], {this, &VirtioCon::msix_xmit_handler});
    Events::get().subscribe(irqs[2], {this, &VirtioCon::event_handler});
  }
  else
  {
    auto irq = Virtio::get_legacy_irq();
    Events::get().subscribe(irq, {this, &VirtioCon::event_handler});
  }

  // Done
  INFO("VirtioCon", "Console with size (%u, %u), %u ports",
       config.cols, config.rows, config.max_nr_ports);
  rx.kick();
}

void VirtioCon::get_config()
{
  Virtio::get_config(&config, sizeof(console_config));
}

void VirtioCon::event_handler()
{
  int isr = hw::inp(iobase() + VIRTIO_PCI_ISR);
  VCPRINT("<VirtioCon> ISR: %#x\n", isr);

  if (isr & 1)
  {
    msix_recv_handler();
    msix_xmit_handler();
  }
  if (isr & 2)
  {
    get_config();
    VCPRINT("\t <VirtioCon> Configuration change: %#x\n", config.status);
  }
}

void VirtioCon::msix_recv_handler()
{
  rx.disable_interrupts();

  while (rx.new_incoming())
  {
    auto res = rx.dequeue();
    if (res.data() != nullptr)
    {
      printf("virtiocon rx: %.*s\n", (int) res.size(), res.data());
    }
    else
    {
      // acknowledgement
      printf("No data, len = %lu\n", res.size());
    }
  }
  rx.enable_interrupts();
}
void VirtioCon::msix_xmit_handler()
{
  tx.disable_interrupts();

  while (tx.new_incoming())
  {
    auto res = tx.dequeue();
    free(res.data());
  }
  tx.enable_interrupts();
}

void VirtioCon::write(const void* data, size_t len)
{
  uint8_t* heapdata = new uint8_t[len];
  memcpy(heapdata, data, len);

  const Token token {{ heapdata, len }, Token::OUT };
  std::array<Token, 1> tokens { token };
  tx.enqueue(tokens);
  tx.kick();
}
