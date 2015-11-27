#include <virtio/virtio.hpp>
#include <kernel/irq_manager.hpp>
#include <kernel/syscalls.hpp>
#include <assert.h>

void Virtio::set_irq(){
  
  //Get device IRQ 
  uint32_t value = _pcidev.read_dword(PCI::CONFIG_INTR);
  if ((value & 0xFF) > 0 && (value & 0xFF) < 32){
    _irq = value & 0xFF;    
  }
  
}


Virtio::Virtio(PCI_Device& dev)
  : _pcidev(dev), _virtio_device_id(dev.product_id() + 0x1040)
{
  INFO("Virtio","Attaching to  PCI addr 0x%x",_pcidev.pci_addr());
  

  /** PCI Device discovery. Virtio std. §4.1.2  */
  
  /** 
      Match vendor ID and Device ID : §4.1.2.2 
  */
  if (_pcidev.vendor_id() != PCI_Device::VENDOR_VIRTIO)
    panic("This is not a Virtio device");
  CHECK(true, "Vendor ID is VIRTIO");
  
  bool _STD_ID = _virtio_device_id >= 0x1040 and _virtio_device_id < 0x107f;
  bool _LEGACY_ID = _pcidev.product_id() >= 0x1000 
    and _pcidev.product_id() <= 0x103f;
  
  CHECK(_STD_ID or _LEGACY_ID, "Device ID 0x%x is in a valid range (%s)",
	_pcidev.product_id(), 
	_STD_ID ? ">= Virtio 1.0" : (_LEGACY_ID ? "Virtio LEGACY" : "INVALID"));
    
  assert(_STD_ID or _LEGACY_ID);
  
  /** 
      Match Device revision ID. Virtio Std. §4.1.2.2 
  */
  bool rev_id_ok = ((_LEGACY_ID and _pcidev.rev_id() == 0) or
                    (_STD_ID and _pcidev.rev_id() > 0));
    
  
  CHECK(rev_id_ok and version_supported(_pcidev.rev_id()), 
	"Device Revision ID (0x%x) supported", _pcidev.rev_id());
  
  assert(rev_id_ok); // We'll try to continue if it's newer than supported.
  
  // Probe PCI resources and fetch I/O-base for device
  _pcidev.probe_resources();
  _iobase=_pcidev.iobase();  
  
  CHECK(_iobase, "Unit has valid I/O base (0x%x)", _iobase);
  
  /** Device initialization. Virtio Std. v.1, sect. 3.1: */
  
  // 1. Reset device
  reset();
  INFO2("[*] Reset device");
  
  // 2. Set ACKNOWLEGE status bit, and
  // 3. Set DRIVER status bit
  
  outp(_iobase + VIRTIO_PCI_STATUS, 
       inp(_iobase + VIRTIO_PCI_STATUS) | 
       VIRTIO_CONFIG_S_ACKNOWLEDGE | 
       VIRTIO_CONFIG_S_DRIVER);
  

  // THE REMAINING STEPS MUST BE DONE IN A SUBCLASS
  // 4. Negotiate features (Read, write, read)
  //    => In the subclass (i.e. Only the Nic driver knows if it wants a mac)  
  // 5. @todo IF >= Virtio 1.0, set FEATURES_OK status bit 
  // 6. @todo IF >= Virtio 1.0, Re-read Device Status to ensure features are OK  
  // 7. Device specifig setup. 
  
  // Where the standard isn't clear, we'll do our best to separate work 
  // between this class and subclasses.
    
  //Fetch IRQ from PCI resource
  set_irq();

  CHECK(_irq, "Unit has IRQ %i", _irq);
  INFO("Virtio","Enabling IRQ Handler");
  enable_irq_handler();


  
  INFO("Virtio", "Initialization complete");
  
  // It would be nice if we new that all queues were the same size. 
  // Then we could pass this size on to the device-specific constructor
  // But, it seems there aren't any guarantees in the standard.
  
  // @note this is "the Legacy interface" according to Virtio std. 4.1.4.8. 
  // uint32_t queue_size = inpd(_iobase + 0x0C);
  
  /* printf(queue_size > 0 and queue_size != PCI_WTF ?
         "\t [x] Queue Size : 0x%lx \n" :
         "\t [ ] No qeuue Size? : 0x%lx \n" ,queue_size); */  
  
}

void Virtio::get_config(void* buf, int len){
  unsigned char* ptr = (unsigned char*)buf;
  uint32_t ioaddr = _iobase + VIRTIO_PCI_CONFIG;
  int i;
  for (i = 0; i < len; i++) *ptr++ = inp(ioaddr + i);
}


void Virtio::reset(){
  outp(_iobase + VIRTIO_PCI_STATUS, 0);
}

uint32_t Virtio::queue_size(uint16_t index){  
  outpw(iobase() + VIRTIO_PCI_QUEUE_SEL, index);
  return inpw(iobase() + VIRTIO_PCI_QUEUE_SIZE);
}

#define BTOP(x) ((unsigned long)(x) >> PAGESHIFT)  
bool Virtio::assign_queue(uint16_t index, uint32_t queue_desc){
  outpw(iobase() + VIRTIO_PCI_QUEUE_SEL, index);
  outpd(iobase() + VIRTIO_PCI_QUEUE_PFN, BTOP(queue_desc));
  return inpd(iobase() + VIRTIO_PCI_QUEUE_PFN) == BTOP(queue_desc);
}

uint32_t Virtio::probe_features(){
  return inpd(_iobase + VIRTIO_PCI_HOST_FEATURES);
}

void Virtio::negotiate_features(uint32_t features){
  _features = inpd(_iobase + VIRTIO_PCI_HOST_FEATURES);
  //_features &= features; //SanOS just adds features
  _features = features;
  debug("<Virtio> Wanted features: 0x%lx \n",_features);
  outpd(_iobase + VIRTIO_PCI_GUEST_FEATURES, _features);
  _features = probe_features();
  debug("<Virtio> Got features: 0x%lx \n",_features);

}

void Virtio::setup_complete(bool ok){
  uint8_t status = ok ? VIRTIO_CONFIG_S_DRIVER_OK : VIRTIO_CONFIG_S_FAILED;
  debug("<VIRTIO> status: %i ",status);
  outp(_iobase + VIRTIO_PCI_STATUS, inp(_iobase + VIRTIO_PCI_STATUS) | status);
}



void Virtio::default_irq_handler(){
  printf("PRIVATE virtio IRQ handler: Call %i \n",calls++);
  printf("Old Features : 0x%x \n",_features);
  printf("New Features : 0x%x \n",probe_features());
  
  unsigned char isr = inp(_iobase + VIRTIO_PCI_ISR);
  printf("Virtio ISR: 0x%i \n",isr);
  printf("Virtio ISR: 0x%i \n",isr);
  
  IRQ_manager::eoi(_irq);
  
}



void Virtio::enable_irq_handler(){
  //_irq=0; //Works only if IRQ2INTR(_irq), since 0 overlaps an exception.
  
  //auto del=delegate::from_method<Virtio,&Virtio::default_irq_handler>(this);  
  auto del(delegate<void()>::from<Virtio,&Virtio::default_irq_handler>(this));
  
  IRQ_manager::subscribe(_irq,del);
  
  IRQ_manager::enable_irq(_irq);
  
  
}

/** void Virtio::enable_irq_handler(IRQ_manager::irq_delegate d){
  //_irq=0; //Works only if IRQ2INTR(_irq), since 0 overlaps an exception.
  //IRQ_manager::set_handler(IRQ2INTR(_irq), irq_virtio_entry);
  
  IRQ_manager::subscribe(_irq,d);
  
  IRQ_manager::enable_irq(_irq);
  }*/




