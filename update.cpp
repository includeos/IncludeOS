/**
 * Master thesis
 * by Alf-Andre Walla 2016-2017
 * 
**/
#include "liveupdate.hpp"

#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include "elf.h"
#include "storage.hpp"

//#define LPRINT(x, ...) printf(x, ##__VA_ARGS__);
#define LPRINT(x, ...) /** x **/

static const int SECT_SIZE   = 512;
static const int ELF_MINIMUM = 164;

static void* HOTSWAP_AREA = (void*) 0x8000;
extern "C" void  hotswap(const char*, int, char*, uintptr_t, void*);
extern "C" char  __hotswap_length;
extern "C" void* __os_store_soft_reset(const void*, size_t);
extern char _ELF_START_;
extern char* heap_begin;
extern char* heap_end;

using namespace liu;

bool LiveUpdate::is_resumable(void* location)
{
  /// memory sanity check
  if (heap_end >= (char*) location) {
    fprintf(stderr, 
        "WARNING: LiveUpdate storage area inside heap (margin: %d)\n",
		heap_end - (char*) location);
    return false;
  }
  return ((storage_header*) location)->validate();
}
bool LiveUpdate::resume(void* location, resume_func func)
{
  // check if an update has occurred
  if (!is_resumable(location)) return false;
  
  LPRINT("* Restoring data...\n");
  // restore connections etc.
  extern bool resume_begin(storage_header&, LiveUpdate::resume_func);
  return resume_begin(*(storage_header*) location, func);
}

static size_t update_store_data(void* location, LiveUpdate::storage_func, buffer_len);

inline bool validate_elf_header(const Elf32_Ehdr* hdr)
{
    return hdr->e_ident[0] == 0x7F &&
           hdr->e_ident[1] == 'E'  &&
           hdr->e_ident[2] == 'L'  &&
           hdr->e_ident[3] == 'F';
}

void LiveUpdate::begin(void*        location, 
                       buffer_len   blob, 
                       storage_func storage_callback)
{
  // use area provided to us directly, which we will assume
  // is far enough into heap to not get overwritten by hotswap.
  // even then, it's still guaranteed to work: the copy mechanism
  // is implemented in hotswap.cpp and copies forwards. the
  // blobs are separated by at least one old kernel size and
  // some early heap allocations, which is at least 1mb, while
  // the copy mechanism just copies single bytes.
  const char* update_area  = blob.buffer;
  char* storage_area = (char*) location;
  
  // validate not overwriting heap, kernel area and other things
  if (storage_area < (char*) 0x200) {
    throw std::runtime_error("The storage area is probably a null pointer");
  }
  if (storage_area >= &_ELF_START_ && storage_area < heap_begin) {
    throw std::runtime_error("The storage area is inside kernel area");
  }
  if (storage_area >= heap_begin && storage_area < heap_end) {
    throw std::runtime_error("The storage area is inside the heap area");
  }

  // validate ELF header
  const char* binary  = &update_area[0];
  const auto* hdr = (const Elf32_Ehdr*) binary;

  if (!validate_elf_header(hdr))
  {
    /// try again with 1 sector offset (skip bootloader)
    binary   = &update_area[SECT_SIZE];
    hdr      = (const Elf32_Ehdr*) binary;
    
    if (!validate_elf_header(hdr))
    {
      /// failed to find elf header at sector 0 and 1
      /// simply return
      throw std::runtime_error("Could not find any ELF header in blob");
    }
  }
  LPRINT("* Found ELF header\n");

  /// note: this assumes section headers are at the end
  int expected_total = 
      hdr->e_shnum * hdr->e_shentsize +
      hdr->e_shoff;

  if (blob.length < expected_total || expected_total < ELF_MINIMUM)
  {
    fprintf(stderr,
        "*** There was a mismatch between blob length and expected ELF file size:\n");
    fprintf(stderr,
        "EXPECTED: %u byte\n",  expected_total);
    fprintf(stderr,
        "ACTUAL:   %u bytes\n", blob.length);
    throw std::runtime_error("ELF file was incomplete");
  }
  LPRINT("* Validated ELF header\n");

  // discover _start() entry point
  const uintptr_t start_offset = hdr->e_entry;
  LPRINT("* _start is located at %#x\n", start_offset);

  // save ourselves if function passed
  if (storage_callback)
  {
      auto storage_len = 
          update_store_data(storage_area, storage_callback, blob);
      (void) storage_len;
  }

  // store soft-resetting stuff
  extern buffer_len get_rollback_location();
  auto  rollback = get_rollback_location();
  void* sr_data  = __os_store_soft_reset(rollback.buffer, rollback.length);
  //void* sr_data = nullptr;

  // get offsets for the new service from program header
  Elf32_Phdr* phdr = (Elf32_Phdr*) &binary[hdr->e_phoff];
  const char* bin_data  = &binary[phdr->p_offset];
  const int   bin_len   = phdr->p_filesz;
  char*       phys_base = (char*) phdr->p_paddr;
  
  //char* phys_base = (char*) (start_offset & 0xffff0000);
  LPRINT("* Physical base address is %p...\n", phys_base);

  /// prepare for the end
  // 1. turn off interrupts
  asm volatile("cli");
  // 2. deactivate all PCI devices and mask all MSI-X vectors
  // NOTE: there are some nasty side effects from calling this
  //hw::Devices::deactivate_all();

  // replace ourselves and reset by jumping to _start
  LPRINT("* Replacing self with %d bytes and jumping to %#x\n", bin_len, start_offset);

  // copy hotswapping function to sweet spot
  memcpy(HOTSWAP_AREA, (void*) &hotswap, &__hotswap_length - (char*) &hotswap);

  /// the end
  ((decltype(&hotswap)) HOTSWAP_AREA)(bin_data, bin_len, phys_base, start_offset, sr_data);
}
size_t LiveUpdate::store(void* location, storage_func func)
{
  return update_store_data(location, func, {nullptr, 0});
}

size_t update_store_data(void* location, LiveUpdate::storage_func func, buffer_len blob)
{
  // create storage header in the fixed location
  new (location) storage_header();
  auto* storage = (storage_header*) location;
  
  /// callback for storing stuff
  Storage wrapper {*storage};
  func(wrapper, blob);
  
  /// finalize
  storage->finalize();
  
  /// sanity check
  if (storage->validate() == false)
      throw std::runtime_error("Failed sanity check on user storage data");

  /// return length of the whole thing
  return storage->total_bytes();
}

/// struct Storage

void Storage::add_marker(uid id)
{
  hdr.add_marker(id);
}
void Storage::add_int(uid id, int value)
{
  hdr.add_int(id, value);
}
void Storage::add_string(uid id, const std::string& str)
{
  hdr.add_string(id, str);
}
void Storage::add_buffer(uid id, buffer_len blob)
{
  hdr.add_buffer(id, blob.buffer, blob.length);
}
void Storage::add_buffer(uid id, const void* buf, size_t len)
{
  hdr.add_buffer(id, (const char*) buf, len);
}
void Storage::add_vector(uid id, const void* buf, size_t count, size_t esize)
{
  hdr.add_vector(id, buf, count, esize);
}
void Storage::add_string_vector(uid id, const std::vector<std::string>& vec)
{
  hdr.add_string_vector(id, vec);
}

#include "serialize_tcp.hpp"
void Storage::add_connection(uid id, Connection_ptr conn)
{
  hdr.add_struct(TYPE_TCP, id,
  [&conn] (char* location) -> int {
    // return size of all the serialized data
    return conn->serialize_to(location);
  });
}

buffer_len buffer_len::deep_copy() const
{
  char* newbuffer = new char[this->length];
  memcpy(newbuffer, this->buffer, this->length);
  return { newbuffer, this->length };
}
