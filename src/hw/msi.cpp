#include <hw/msi.hpp>

#include <hw/pci_device.hpp>
#include <info>

//#define VERBOSE_MSIX
#ifdef VERBOSE_MSIX
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif

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
  inline void mm_write(intptr_t addr, uint32_t val)
  {
    *((uint32_t volatile*) addr) = val;
  }
  inline uint32_t mm_read (intptr_t addr)
  {
    return *(uint32_t volatile*) addr;
  }
  inline uint32_t msix_addr_for_cpu(uint8_t cpu_id)
  {
    // send to a specific APIC ID
    const uint32_t RH = 1;
    const uint32_t DM = 0;
    // create message address
    return 0xfee00000 | (cpu_id << 12) | (RH << 3) | (DM << 2);
  }
  inline uint32_t msix_data_single_vector(uint8_t vector)
  {
    assert(vector >= 32 && vector != 0xff);
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
    auto bar = dev.read32(offset);

    auto capbar_off = bar & ~MSIX_BIR_MASK;
    bar &= MSIX_BIR_MASK;

    if (dev.validate_bar(bar) == false)
    {
      printf("PCI: Invalid BAR: %u\n", bar);
      return 0;
    }
    auto pcibar = dev.get_bar(bar);
    assert(pcibar.start != 0);

    PRINT("[MSI-X] offset %p -> bir %u => %p  res: %p\n",
          (void*) offset, bar, (void*) pcibar.start,
          (void*) (pcibar.start + capbar_off));
    return pcibar.start + capbar_off;
  }

  msix_t::msix_t(PCI_Device& device, uint32_t cap)
    : dev(device)
  {
    // validate capability structure
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
    PRINT("[MSI-X] Table addr: %p  PBA addr: %p\n",
          (void*) this->table_addr, (void*) this->pba_addr);

    if (this->table_addr == 0) return;
    if (this->pba_addr == 0) return;

    // get number of vectors we can get notifications from
    const size_t vector_cnt = (func & MSIX_TBL_SIZE) + 1;

    if (vector_cnt > 2048) {
      printf("table addr: %p  pba addr: %p  vectors: %zu\n",
              (void*) table_addr, (void*) pba_addr, vectors());
      printf("Unreasonably many MSI-X vectors!");
      return;
    }
    used_vectors.resize(vector_cnt);

    // manually mask all entries
    for (size_t i = 0; i < this->vectors(); i++) {
      mask_entry(i);
    }
    PRINT("[MSI-X] Enabled with %zu vectors\n", this->vectors());

    // unmask vectors
    func &= ~MSIX_FUNC_MASK;
    // write back message control bits
    dev.write16(cap + 2, func);
  }

  uint16_t msix_t::setup_vector(uint8_t cpu, uint8_t intr)
  {
    // find free table entry
    uint16_t vec;
    for (vec = 0; vec < used_vectors.size(); vec++)
    {
      if (used_vectors[vec] == false) break;
    }
    assert (vec != this->vectors());
    // use free table entry
    redirect_vector(vec, cpu, intr);
    // return it
    return vec;
  }

  void msix_t::redirect_vector(uint16_t idx, uint8_t cpu, uint8_t intr)
  {
    assert(idx < vectors());
    INFO2("MSI-X vector %u pointing to cpu %u intr %u", idx, cpu, intr);

    mask_entry(idx);
    mm_write(get_entry(idx, ENT_MSG_ADDR), msix_addr_for_cpu(cpu));
    mm_write(get_entry(idx, ENT_MSG_UPPER), 0x0);
    mm_write(get_entry(idx, ENT_MSG_DATA), msix_data_single_vector(intr));
    unmask_entry(idx);
    // mark as being used
    this->used_vectors.at(idx) = true;
  }
}
