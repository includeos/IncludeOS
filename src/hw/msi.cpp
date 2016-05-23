#include <hw/msi.hpp>
#include <hw/pci_device.hpp>

#define MSI_ENABLE     0x1
#define MSIX_ENABLE    (1 << 15)
#define MSIX_MASK      (1 << 14)
#define MSIX_TBL_SIZE  0x7ff

namespace hw
{
  
  void mm_write(intptr_t addr, uint32_t val)
  {
    *((uint32_t volatile*) addr) = val;
  }
  uint32_t mm_read (intptr_t addr)
  {
    return *(uint32_t volatile*) addr;
  }
  
  void msix_t::mask_entry(size_t vec)
  {
    auto reg = (uintptr_t) &get_entry(vec)->vector;
    mm_write(reg, mm_read(reg) | 0x1);
  }
  void msix_t::unmask_entry(size_t vec)
  {
    auto reg = (uintptr_t) &get_entry(vec)->vector;
    mm_write(reg, mm_read(reg) & ~0x1);
  }
  
  intptr_t msix_t::get_capbar_paddr(size_t offset)
  {
    auto bir = dev.read_dword(offset);
    auto capbar_off = bir & ~0x7;
    bir &= 0x7;
    printf("MSI-X: off %d bir %d\n", capbar_off, bir);
    
    auto membar = dev.get_membar(bir);
    
    printf("membar: %#x  membar off: %#x\n",
        membar, membar + capbar_off);
    assert(membar != 0);
    
    membar += capbar_off;
    return membar;
  }
  
  msix_t::msix_t(PCI_Device& device)
    : dev(device)
  {
    auto cap = dev.msix_cap();
    auto func = dev.read16(cap + 2);
    printf("func: %#x\n", func);
    // enable msix and mask the device
    func |= MSIX_ENABLE | MSIX_MASK;
    dev.write16(cap + 2, func);
    
    // get the physical addresses of the
    // MSI-X table and pending bit array (PBA)
    this->table_addr = get_capbar_paddr(cap + 4);
    this->pba_addr   = get_capbar_paddr(cap + 8);
    // get number of vectors we can get notifications from
    this->vectors = (func & MSIX_TBL_SIZE) + 1;
    
    printf("table addr: %#x  pba addr: %#x  vectors: %u\n",
      table_addr, pba_addr, vectors);
    
    // mask out all entries
    for (size_t i = 0; i < vectors; i++)
      mask_entry(i);
    
    // unmask device
    func &= ~MSIX_MASK;
    dev.write16(cap + 2, func);
  }
}
