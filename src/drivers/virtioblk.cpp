#include "virtioblk.hpp"

#include <kernel/events.hpp>
#include <fs/common.hpp>
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

// a deleter that does nothing
void null_deleter(uint8_t*) {};

#include <statman>

VirtioBlk::VirtioBlk(hw::PCI_Device& d)
  : Virtio(d), hw::Block_device(), req(device_name() + ".req0", queue_size(0), 0, iobase()), inflight(0)
{
  INFO("VirtioBlk", "Initializing");
  {
    auto& reqs = Statman::get().create(
      Stat::UINT32, device_name() + ".requests");
    this->requests = &reqs.get_uint32();
    *this->requests = 0;

    auto& err = Statman::get().create(
      Stat::UINT32, device_name() + ".errors");
    this->errors = &err.get_uint32();
    *this->errors = 0;
  }

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
  auto success = assign_queue(0, req.queue_desc());
  CHECK(success, "Request queue assigned (%p) to device",
        req.queue_desc());

  // Step 3 - Fill receive queue with buffers
  // DEBUG: Disable
  INFO("VirtioBlk", "Queue size: %i\tRequest size: %zu\n",
       req.size(), sizeof(request_t));

  // Get device configuration
  get_config();

  // Signal setup complete.
  setup_complete((features() & needed_features) == needed_features);
  CHECK((features() & needed_features) == needed_features, "Signalled driver OK");

  // Hook up IRQ handler (inherited from Virtio)
  if (has_msix())
  {
    assert(get_msix_vectors() >= 2);
    auto& irqs = this->get_irqs();
    // update IRQ subscriptions
    Events::get().subscribe(irqs[0], {this, &VirtioBlk::service_RX});
    Events::get().subscribe(irqs[1], {this, &VirtioBlk::msix_conf_handler});
  }
  else
  {
    auto& irqs = this->get_irqs();
    Events::get().subscribe(irqs[0], {this, &VirtioBlk::irq_handler});
  }

  // Done
  INFO("VirtioBlk", "Block device with %zu sectors capacity", config.capacity);
}

void VirtioBlk::get_config()
{
  Virtio::get_config(&config, sizeof(virtio_blk_config_t));
}

void VirtioBlk::msix_conf_handler()
{
  debug("\t <VirtioBlk> Configuration change:\n");
  get_config();
}

void VirtioBlk::irq_handler() {

  debug2("<VirtioBlk> IRQ handler\n");

  //Virtio Std. § 4.1.5.5, steps 1-3

  // Step 1. read ISR
  unsigned char isr = hw::inp(iobase() + VIRTIO_PCI_ISR);

  // Step 2. A) - one of the queues have changed
  if (isr & 1) {
    // This now means service RX & TX interchangeably
    service_RX();
  }

  // Step 2. B)
  if (isr & 2) {
    debug("\t <VirtioBlk> Configuration change:\n");

    // Getting the MAC + status
    //debug("\t             Old status: 0x%x\n", config.status);
    get_config();
    //debug("\t             New status: 0x%x \n", config.status);
  }
}

void VirtioBlk::handle(request_t* hdr)
{
  // check request response
  blk_resp_t& resp = hdr->resp;
  // only call handler with data when the request was fullfilled
  //printf("response: status %u blk %llu  func %p  buff %p\n",
  //      resp.status, hdr->hdr.sector, resp.handler.get_ptr(), hdr->io.sector);

  if (resp.status == 0) {
    // packaged as buffer, but deleted as request
    resp.handler(hdr->io.sector);
  }
  else {
    // return empty shared ptr
    resp.handler(nullptr);
  }

  // delete request
  delete hdr;
}

void VirtioBlk::service_RX()
{
  req.disable_interrupts();
  while (req.new_incoming())
  {
    auto tok = req.dequeue();
    if (!tok.size()) break;

    // only handle the main header of each request
    auto* hdr = (request_t*) tok.data();
    received.emplace_back(hdr);
  };

  // if we have free space and jobs, start shipping
  bool shipped = false;
  while (free_space() && !jobs.empty()) {
    shipit(jobs.front());
    jobs.pop_front();
    shipped = true;
  }
  if (shipped) req.kick();
  req.enable_interrupts();

  int handled = 0;
  for (request_t* hdr : received) {
    handle(hdr);
    inflight--; handled++;
  }
  received.clear();

  //printf("inflight: %d  handled: %d  shipped: %d  num_free: %u\n",
  //    inflight, handled, scnt, req.num_free());
}

void VirtioBlk::shipit(request_t* vbr) {

  Token token1 { { (uint8_t*) &vbr->hdr, sizeof(scsi_header_t) }, Token::OUT };
  Token token2 { { (uint8_t*) &vbr->io, sizeof(blk_io_t) }, Token::IN };
  Token token3 { { (uint8_t*) &vbr->resp, 1 }, Token::IN }; // 1 status byte

  //printf("shipping job: sect %llu  func %p  buff %p\n",
  //      vbr->hdr.sector, vbr->resp.handler.get_ptr(), vbr->io.sector);
  std::array<Token, 3> tokens {{ token1, token2, token3 }};
  req.enqueue(tokens);
  inflight++;
  (*this->requests)++;
}

void VirtioBlk::read (block_t blk, size_t cnt, on_read_func func)
{
  // create big buffer for collecting all the disk data
  auto bigbuf = fs::construct_buffer(block_size() * cnt);
  // number of reads left
  auto results = std::make_shared<size_t> (cnt);
  bool shipped = false;
  //printf("virtioblk: Enqueue blk %llu cnt %u\n", blk, cnt);

  //for (int i = cnt-1; i >= 0; i--)
  for (size_t i = 0; i < cnt; i++)
  {
    // create a special request where we collect all the data
    auto* vbr = new request_t(
      blk + i,
      request_handler_t::make_packed(
      [this, i, func, results, bigbuf] (uint8_t* data) {
        // if the job was already completed, return early
        if (*results == 0) {
          printf("[virtioblk] Job cancelled? results == 0,  blk=%zu\n", i);
          return;
        }
        // validate partial result
        if (data != nullptr) {
          *results -= 1;
          // copy partial block
          memcpy(bigbuf->data() + i * block_size(), data, block_size());
          // check if we have all blocks
          if (*results == 0) {
            // finally, call user-provided callback
            func(bigbuf);
          }
        }
        else {
          (*this->errors)++;
          // if the partial result failed, cancel all
          *results = 0;
          // callback with no data
          func(nullptr);
        }
      })
    );
    //printf("virtioblk: Enqueue blk %llu\n", blk + i);
    //
    if (free_space()) {
      shipit(vbr);
      shipped = true;
    }
    else
      jobs.push_back(vbr);
  }
  // kick when we have enqueued stuff
  if (shipped) req.kick();
}

VirtioBlk::request_t::request_t(uint64_t blk, request_handler_t cb)
{
  hdr.type   = VIRTIO_BLK_T_IN;
  hdr.ioprio = 0; // reserved
  hdr.sector = blk;
  resp.status  = VIRTIO_BLK_S_IOERR;
  resp.handler = cb;
}

void VirtioBlk::deactivate()
{
  /// disable interrupts on virtio queues
  req.disable_interrupts();

  /// reset device
  this->Virtio::reset();
}

#include <kernel/pci_manager.hpp>

/** Global constructor - register VirtioBlk's driver factory at the PCI_manager */
struct Autoreg_virtioblk {
  Autoreg_virtioblk() {
    PCI_manager::register_blk(PCI::VENDOR_VIRTIO, 0x1001, &VirtioBlk::new_instance);
  }
} autoreg_virtioblk;
