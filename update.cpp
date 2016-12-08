#include "update.hpp"

#include <cassert>
#include <cstdint>
#include <string>
#include <unistd.h>
#include "elf.h"
#include "storage.hpp"

static const int SECT_SIZE   = 512;
static const int ELF_MINIMUM = 164;

static void* HOTSWAP_AREA = (void*) 0x8000;

bool LiveUpdate::is_resumable(void* location)
{
  return ((storage_header*) location)->validate();
}
bool LiveUpdate::resume(void* location, resume_func func)
{
  // check if an update has occurred
  if (!is_resumable(location)) return false;
  
  printf("* Restoring data...\n");
  // restore connections etc.
  extern bool resume_begin(storage_header&, LiveUpdate::resume_func);
  return resume_begin(*(storage_header*) location, func);
}

static void update_store_data(void* location, LiveUpdate::storage_func, buffer_len);

extern "C" void  hotswap(const char*, int, char*, uintptr_t, void*);
extern "C" char  __hotswap_length;
extern "C" void* __os_store_soft_reset();

#include <hw/devices.hpp>

inline bool validate_elf_header(Elf32_Ehdr* hdr)
{
    return hdr->e_ident[0] == 0x7F &&
           hdr->e_ident[1] == 'E'  &&
           hdr->e_ident[2] == 'L'  &&
           hdr->e_ident[3] == 'F';
}

void LiveUpdate::begin(void* location, buffer_len blob, storage_func func)
{
  // use area just below the update storage to
  // prevent it getting overwritten during the update
  char* storage_area = (char*) location;
  char* update_area  = storage_area - blob.length;
  memcpy(update_area, blob.buffer, blob.length);

  // validate ELF header
  char*   binary  = &update_area[0];
  int     bin_len = blob.length;
  Elf32_Ehdr* hdr = (Elf32_Ehdr*) binary;

  if (!validate_elf_header(hdr))
  {
    /// try again with 1 sector offset (skip bootloader)
    binary   = &update_area[SECT_SIZE];
    bin_len  = blob.length - SECT_SIZE;
    hdr      = (Elf32_Ehdr*) binary;
    
    if (!validate_elf_header(hdr))
    {
      /// failed to find elf header at sector 0 and 1
      /// simply return
      printf("*** Failed to find any ELF header in blob\n");
      return;
    }
  }
  printf("* Found ELF header\n");
  
  int expected_total = 
      //hdr->e_ehsize + 
      //hdr->e_phnum * hdr->e_phentsize + 
      hdr->e_shnum * hdr->e_shentsize +
      hdr->e_shoff; /// this assumes section headers are at the end
  
  if (blob.length < expected_total || expected_total < ELF_MINIMUM)
  {
    printf("*** There was a mismatch between blob length and expected ELF file size:\n");
    printf("EXPECTED: %u byte\n",  expected_total);
    printf("ACTUAL:   %u bytes\n", blob.length);
    return;
  }
  printf("* Validated ELF header\n");

  // discover _start() entry point
  const uintptr_t start_offset = hdr->e_entry;
  printf("* _start is located at %#x\n", start_offset);

  // save ourselves
  update_store_data(storage_area, func, {update_area, blob.length});

  // store soft-resetting stuff
  void* sr_data = __os_store_soft_reset();
  //void* sr_data = nullptr;

  // try to guess base address for the new service based on entry point
  /// FIXME
  char* phys_base = (char*) 0x100000; //(start_offset & 0xffff0000);
  printf("* Estimate physical base to be %p...\n", phys_base);

  /// prepare for the end
  // 1. turn off interrupts
  asm volatile("cli");
  // 2. deactivate all PCI devices and mask all MSI-X vectors
  // NOTE: there are some nasty side effects from calling this
  //hw::Devices::deactivate_all();

  // replace ourselves and reset by jumping to _start
  printf("* Replacing self with %d bytes and jumping to %#x\n", bin_len, start_offset);

  // copy hotswapping function to sweet spot
  memcpy(HOTSWAP_AREA, (void*) &hotswap, &__hotswap_length - (char*) &hotswap);

  /// the end
  ((decltype(&hotswap)) HOTSWAP_AREA)(binary, bin_len, phys_base, start_offset, sr_data);
}

void update_store_data(void* location, LiveUpdate::storage_func func, buffer_len blob)
{
  // create storage header in the fixed location
  new (location) storage_header();
  auto* storage = (storage_header*) location;
  
  /// callback for storing stuff
  func({*storage}, blob);
  
  /// finalize
  storage->finalize();
  
  /// sanity check
  assert(storage->validate());
}

/// struct Storage

void Storage::add_string(uint16_t id, const std::string& string)
{
  hdr.add_string(id, string);
}
void Storage::add_buffer(uint16_t id, buffer_len blob)
{
  hdr.add_buffer(id, blob.buffer, blob.length);
}
void Storage::add_buffer(uint16_t id, void* buf, size_t len)
{
  hdr.add_buffer(id, (char*) buf, len);
}

#include "serialize_tcp.hpp"
void Storage::add_connection(uid id, Connection conn)
{
  hdr.add_struct(TYPE_TCP, id,
  [&conn] (char* location) -> int {
    // return size of all the serialized data
    return conn->serialize_to(location);
  });
}
