#include <hw/msi.hpp>

#include <hw/pci_device.hpp>
#include <info>

#define MSI_ENABLE     0x1
#define MSIX_ENABLE    (1 << 15)
#define MSIX_FUNC_MASK (1 << 14)
#define MSIX_TBL_SIZE  0x7ff
#define MSIX_BIR_MASK  0x7
#define MSIX_ENTRY_SIZE       16
#define MSIX_ENTRY_CTL_MASK   0x1


#define ENT_VECTOR_CTL  12
#define ENT_MSG_DATA     8
#define ENT_MSG_UPPER    4
#define ENT_MSG_ADDR     0

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
  uint32_t msix_addr_single_cpu(uint8_t cpu_id)
  {
    // send to a specific APIC ID
    const uint32_t RH = 1;
    const uint32_t DM = 0;
    // create message address
    return 0xfee00000 | (cpu_id << 12) | (RH << 3) | (DM << 2);
  }
  uint32_t msix_data_single_vector(uint8_t vector)
  {
    // range 16 >= vector > 255
    assert((vector & 0xF0) && vector != 0xff);
    const uint32_t DM = 0x1; // low-pri
    const uint32_t TM = 0;
    
    return (TM << 15) | (DM << 8) | vector;
  }
  
  inline uintptr_t msix_t::get_entry(size_t idx, size_t offs)
  {
    return table_addr + idx * MSIX_ENTRY_SIZE + offs;
  }
  inline uintptr_t msix_t::get_pba(size_t chunk)
  {
    return pba_addr + sizeof(uintptr_t) * chunk;
  }
  
  void msix_t::mask_entry(size_t vec)
  {
    auto reg = get_entry(vec, ENT_VECTOR_CTL);
    mm_write(reg, mm_read(reg) | MSIX_ENTRY_CTL_MASK);
  }
  void msix_t::unmask_entry(size_t vec)
  {
    auto reg = get_entry(vec, ENT_VECTOR_CTL);
    mm_write(reg, mm_read(reg) & ~MSIX_ENTRY_CTL_MASK);
  }
  void msix_t::zero_entry(size_t vec)
  {
    auto reg = get_entry(vec, ENT_MSG_DATA);
    mm_write(reg, mm_read(reg) & ~0xff);
  }
  
  void msix_t::reset_pba_bit(size_t vec)
  {
    auto chunk = vec / 32;
    auto bit   = vec & 31;
    
    auto reg = get_pba(chunk);
    mm_write(reg, mm_read(reg) & ~(1 << bit));
  }
  
  uintptr_t msix_t::get_bar_paddr(size_t offset)
  {
    /**
     * 6.8.3.2  MSI-X Configuration
     * Software calculates the base address of the MSI-X Table by reading the 32-bit 
     * value from the Table Offset / Table BIR register, masking off the lower 
     * 3 Table BIR bits, and adding the remaining QWORD-aligned 32-bit Table offset
     * to the address taken from the Base Address register indicated by the Table BIR. 
     * Software calculates the base address of the MSI-X PBA using the same process 
     * with the PBA Offset / PBA BIR register.
    **/
    auto bar = dev.read_dword(offset);
    auto capbar_off = bar & ~MSIX_BIR_MASK;
    bar &= MSIX_BIR_MASK;
    
    auto baroff = dev.get_membar(bar);
    assert(baroff != 0);
    
    return capbar_off + baroff;
  }
  
  msix_t::msix_t(PCI_Device& device)
    : dev(device)
  {
    // get capability structure
    auto cap = dev.msix_cap();
    assert(cap >= 0x40);
    // read message control bits
    uint16_t func = dev.read16(cap + 2);
    
    /// if MSIX was already enabled, avoid validating func
    if ((func & MSIX_ENABLE) == 0)
      assert(func < 0x1000 && "Invalid MSI-X func read");
    
    // enable msix and mask all vectors
    func |= MSIX_ENABLE | MSIX_FUNC_MASK;
    dev.write16(cap + 2, func);
    
    // get the physical addresses of the
    // MSI-X table and pending bit array (PBA)
    this->table_addr = get_bar_paddr(cap + 4);
    this->pba_addr   = get_bar_paddr(cap + 8);
    // get number of vectors we can get notifications from
    this->vector_cnt = (func & MSIX_TBL_SIZE) + 1;
    
    if (vector_cnt > 16) {
      printf("table addr: %#x  pba addr: %#x  vectors: %u\n",
              table_addr, pba_addr, vectors());
      assert(vectors() <= 16 && "Unreasonably many MSI-X vectors");
    }
    
    // reset all entries
    for (size_t i = 0; i < this->vectors(); i++) {
      mask_entry(i);
      zero_entry(i);
    }
    
    // unmask vectors
    func &= ~MSIX_FUNC_MASK;
    // write back message control bits
    dev.write16(cap + 2, func);
  }
  
  uint16_t msix_t::setup_vector(uint8_t cpu, uint8_t intr)
  {
    // find free table entry
    uint16_t vec;
    for (vec = 0; vec < this->vectors(); vec++)
    {
      // read data register
      auto reg = get_entry(vec, ENT_MSG_DATA);
      if ((mm_read(reg) & 0xff) == 0) break;
    }
    assert (vec != this->vectors());
    
    // use free table entry
    INFO2("MSI-X vector %u pointing to cpu %u intr %u",
        vec, cpu, intr);
    
    // mask entry
    mask_entry(vec);
    
    mm_write(get_entry(vec, ENT_MSG_ADDR), msix_addr_single_cpu(cpu));
    mm_write(get_entry(vec, ENT_MSG_UPPER), 0x0);
    mm_write(get_entry(vec, ENT_MSG_DATA), msix_data_single_vector(intr));
    
    // unmask entry
    unmask_entry(vec);
    // return it
    return vec;
  }
  
}
