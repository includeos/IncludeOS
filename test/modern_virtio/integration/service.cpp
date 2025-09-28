#include <os>
#include <stdio.h>
#include <memory>

#include <hw/pci_device.hpp>
#include <hw/pci_manager.hpp>

#include <kernel/events.hpp>
#include <modern_virtio/control_plane.hpp>
#include <modern_virtio/split_queue.hpp>

#define CON_ID 0x1043
#define VIRTCON_ID (PCI::VENDOR_VIRTIO | (CON_ID << 16))

std::unique_ptr<hw::PCI_Device> console_pcidev;
std::unique_ptr<Virtio_control> virt_control;
std::unique_ptr<Split_queue> xmit_queue;

auto send_lambda = []() {
	xmit_queue->dequeue();

	virt_control->deactivate_virtio_control();
	printf("SUCCESS\n");
	os::shutdown();
};

int main() {
	/* Creating a generic PCI device */
	bool found_device = false;
	for (const auto& [pci_addr, id, devclass]: hw::PCI_manager::devinfos()) {
		if (id == VIRTCON_ID) {
			found_device = true;
			console_pcidev = std::make_unique<hw::PCI_Device>(pci_addr, id, devclass.reg);
		}
	}
	Expects(found_device == true);

	/* Creating Virtio control plane object */
	virt_control = std::make_unique<Virtio_control>(*console_pcidev);
	virt_control->negotiate_features(0, 0);

	/* Enabling MSIX interrupts */
	virt_control->enable_msix(1);

	/* Creating an xmit queue */
	xmit_queue = std::make_unique<Split_queue>(*virt_control, 5, false, 0);
	auto event_num = Events::get().subscribe(nullptr);
  Events::get().subscribe(event_num, {send_lambda});
  console_pcidev->setup_msix_vector(0, IRQ_BASE + event_num);

	/* Finalizing the initialization */
	virt_control->set_driver_ok_bit();

	/* Suppressing interrupts to check queue without interrupts  */
	xmit_queue->suppress();

	/* Sending a chained descriptor chain */
	VirtTokens send_tokens0;
	for (int i = 0; i < 4; ++i) {
		send_tokens0.emplace_back(
			VIRTQ_DESC_F_NOFLAGS,
			(uint8_t*)"Riegrot\n",
			8
		);
	}
	xmit_queue->enqueue(send_tokens0);
	xmit_queue->kick();
	while(xmit_queue->has_processed_used());
	Expects(xmit_queue->dequeue().size() == 4);
	Expects(xmit_queue->desc_space_cap() == xmit_queue->free_desc_space());
	printf("Sent a chained buffer!\n");

	/* Having multiple chains in flight */
	VirtTokens send_tokens1;
	send_tokens1.emplace_back(
		VIRTQ_DESC_F_NOFLAGS,
		(uint8_t*)"Neram\n",
		6
	);
	send_tokens1.emplace_back(
		VIRTQ_DESC_F_NOFLAGS,
		(uint8_t*)"Dnuma\n",
		6
	);
	for (int i = 0; i < 5; ++i) {
		xmit_queue->enqueue(send_tokens1);
	}

	xmit_queue->kick();
	Expects((xmit_queue->desc_space_cap() - 10) == xmit_queue->free_desc_space());

	while(xmit_queue->free_desc_space() < xmit_queue->desc_space_cap()) {
		while(xmit_queue->has_processed_used());
		xmit_queue->dequeue();
	}
	Expects(xmit_queue->desc_space_cap() == xmit_queue->free_desc_space());
	printf("Sent 5 chained buffer chains successfully!\n");

	/* Sending hundred of messages */
	VirtTokens send_tokens2;
	send_tokens2.emplace_back(
		VIRTQ_DESC_F_NOFLAGS,
		(uint8_t*)"ebutuoy\n",
		8
	);

	for (int i = 0; i < 300; ++i) {
		xmit_queue->enqueue(send_tokens2);
		xmit_queue->kick();
		while(xmit_queue->has_processed_used());
		xmit_queue->dequeue();
	}
	Expects(xmit_queue->desc_space_cap() == xmit_queue->free_desc_space());
	printf("Sent hundreds of messages successfully!\n");

	/* Reenabling interrupts and checking that interrupts/callback work */
	xmit_queue->unsuppress();
	xmit_queue->enqueue(send_tokens2);
	xmit_queue->kick();
}
