#include "update.hpp"

#include <cassert>
#include <cstdint>
#include <string>
#include "elf.h"
#include "storage.hpp"
#include "resume.hpp"

static const int SERV_OFFSET  = 6; // seems like an unintended offset
static const uintptr_t UPDATE_STORAGE = 0x3000000; // at 48mb

void liveupdate_resume()
{
  // check if an update has occurred
  if (*(int64_t*) UPDATE_STORAGE == 0xbaadb33f)
  {
    // restore connections etc.
    resume_begin();
  } else {
    printf("Not restoring data, because no update has happened\n");
  }
}

static void begin_update(const char* blob, size_t size);

void liveupdate_begin(net::tcp::Connection_ptr conn)
{
  static char*  update_blob = new char[1024*1024*10];
  static size_t update_size = 0;

  // reset update chunk
  update_size = 0;
  // retrieve binary
  conn->on_read(9000,
  [conn] (net::tcp::buffer_t buf, size_t n)
  {
    memcpy(update_blob + update_size, buf.get(), n);
    update_size += n;

  }).on_close(
  [] {
    printf("* New update size: %u b\n", update_size);
    begin_update(update_blob, update_size);
    // We should never return :-)
    printf("!! Update failed\n");
  });
}

struct SymTab {
  Elf32_Sym* base;
  uint32_t   entries;
};
struct StrTab {
  char*    base;
  uint32_t size;
};
extern "C" void _start();
static void update_store_data();

void begin_update(const char* blob, size_t size)
{
  // allocate 2x @size, which should be enough to
  // prevent it getting overwritten during the update
  char* update_area = new char[2*size];
  update_area += size;
  memcpy(update_area, blob, size);

  // validate ELF header
  const char* binary = (char*)update_area + 512;
  Elf32_Ehdr& hdr = *(Elf32_Ehdr*) binary;

  if (hdr.e_ident[0] != 0x7F ||
      hdr.e_ident[1] != 'E' ||
      hdr.e_ident[2] != 'L' ||
      hdr.e_ident[3] != 'F') return;
  printf("* Validated ELF header\n");

  // discover _start() entry point
  const uintptr_t serv_offset = *(uintptr_t*) &update_area[SERV_OFFSET];
  printf("* _start is located at %#x\n", serv_offset);
  
  // save ourselves
  update_store_data();

  // try to guess base address for the new service based on entry point
  void* phys_base = (void*) (serv_offset & 0xffff8000);
  printf("* Estimate physical base to be %p...\n", phys_base);
  
  // replace ourselves and reset by jumping to _start
  printf("* Jumping to new service...\n\n");
  memcpy(phys_base, binary, size);

  auto entry_func = (decltype(&_start)) serv_offset;
  entry_func();
}

void update_store_data()
{
  // create storage header in the fixed location
  auto* storage = (storage_header*) UPDATE_STORAGE;
  new (storage) storage_header();
  
  /// ...
  auto& entry = storage->create_entry(TYPE_STRING, "some_string", 24);
  /// create string
  snprintf(entry.vla, entry.length, "%s", "Some value for the string");
  
  
}
