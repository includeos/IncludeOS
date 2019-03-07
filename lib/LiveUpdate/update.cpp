// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 IncludeOS AS, Oslo, Norway
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
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
#include <unordered_map>
#include <elf.h>
#include "storage.hpp"
#include <kernel.hpp>
#include <os.hpp>
#include <kernel/memory.hpp>
#include <hw/nic.hpp> // for flushing

#define LPRINT(x, ...) printf(x, ##__VA_ARGS__);
//#define LPRINT(x, ...) /** x **/

static const int SECT_SIZE   = 512;
static const int ELF_MINIMUM = 164;
// hotswapping functions
extern "C" void solo5_exec(const char*, size_t);
static void* HOTSWAP_AREA = (void*) 0x8000;
extern "C" void  hotswap(char*, const char*, int, void*, void*);
extern "C" char  __hotswap_length;
extern "C" void  hotswap64(char*, const char*, int, uint32_t, void*, void*);
extern uint32_t  hotswap64_len;
extern void      __x86_init_paging(void*);
extern "C" void* __os_store_soft_reset(const void*, size_t);
// kernel area
extern char _ELF_START_;
extern char _end;
// turn this off to reduce liveupdate times at the cost of extra checks
bool LIVEUPDATE_USE_CHEKSUMS    = true;
// turn this om to zero-initialize all memory between new kernel and heap end
bool LIVEUPDATE_ZERO_OLD_MEMORY = false;

using namespace liu;

static size_t update_store_data(void* location, const buffer_t*);

// serialization callbacks
static std::unordered_map<std::string, LiveUpdate::storage_func> storage_callbacks;

void LiveUpdate::register_partition(std::string key, storage_func callback)
{
#if defined(USERSPACE_KERNEL)
  // on linux we cant make the jump, so the tracking wont reset
  storage_callbacks[key] = std::move(callback);
#else
  auto it = storage_callbacks.find(key);
  if (it == storage_callbacks.end())
  {
    storage_callbacks.emplace(std::piecewise_construct,
              std::forward_as_tuple(std::move(key)),
              std::forward_as_tuple(std::move(callback)));
  }
  else {
    throw std::runtime_error("Storage key '" + key + "' already used");
  }
#endif
}

template <typename Class>
inline bool validate_header(const Class* hdr)
{
    return hdr->e_ident[0] == 0x7F &&
           hdr->e_ident[1] == 'E'  &&
           hdr->e_ident[2] == 'L'  &&
           hdr->e_ident[3] == 'F';
}

void LiveUpdate::exec(const buffer_t& blob, std::string key, storage_func func)
{
  if (func != nullptr) LiveUpdate::register_partition(key, func);
  LiveUpdate::exec(blob);
}

void LiveUpdate::exec(const buffer_t& blob, void* location)
{
  if (location == nullptr) location = kernel::liveupdate_storage_area();
  LPRINT("LiveUpdate::begin(%p, %p:%d, ...)\n", location, blob.data(), (int) blob.size());
#if defined(__includeos__)
  // 1. turn off interrupts
  asm volatile("cli");
#endif

  // use area provided to us directly, which we will assume
  // is far enough into heap to not get overwritten by hotswap.
  // even then, it's still guaranteed to work: the copy mechanism
  // is implemented in hotswap.cpp and copies forwards. the
  // blobs are separated by at least one old kernel size and
  // some early heap allocations, which is at least 1mb, while
  // the copy mechanism just copies single bytes.
  if (blob.size() < ELF_MINIMUM)
      throw std::runtime_error("Buffer too small to be valid ELF");
  const char* update_area  = blob.data();
  char* storage_area = (char*) location;

  // validate not overwriting heap, kernel area and other things
  if (storage_area < (char*) 0x200) {
    throw std::runtime_error("LiveUpdate storage area is (probably) a null pointer");
  }
#if !defined(PLATFORM_UNITTEST) && !defined(USERSPACE_KERNEL)
  const uintptr_t storage_area_phys = os::mem::virt_to_phys((uintptr_t) storage_area);
  // NOTE: on linux the heap location is randomized,
  // so we could compare against that but: How to get the heap base address?
  if (storage_area >= &_ELF_START_ && storage_area < &_end) {
    throw std::runtime_error("LiveUpdate storage area is inside kernel area");
  }
  if (storage_area >= (char*) kernel::heap_begin() && storage_area < (char*) kernel::heap_end()) {
    throw std::runtime_error("LiveUpdate storage area is inside the heap area");
  }
  if (storage_area_phys >= kernel::heap_max()) {
    throw std::runtime_error("LiveUpdate storage area is outside physical memory");
  }
  /*
  if (storage_area_phys >= kernel::memory_end() - 0x10000) {
    printf("Storage area is at %p / %p\n",
           (void*) storage_area_phys, (void*) kernel::heap_max());
    throw std::runtime_error("LiveUpdate storage area needs at least 64kb memory");
  }*/
#endif

  // search for ELF header
  LPRINT("* Looking for ELF header at %p\n", update_area);
  const char* binary  = &update_area[0];
  const auto* hdr = (const Elf32_Ehdr*) binary;
  if (!validate_header<Elf32_Ehdr>(hdr))
  {
    /// try again with 1 sector offset (skip bootloader)
    binary   = &update_area[SECT_SIZE];
    hdr      = (const Elf32_Ehdr*) binary;

    if (!validate_header<Elf32_Ehdr>(hdr))
    {
      /// failed to find elf header at sector 0 and 1
      /// simply return
      throw std::runtime_error("Could not find any ELF header in blob");
    }
  }
  LPRINT("* Found ELF header\n");

  bool      found_kernel_start = false;
  size_t    expected_total = 0;
  uint32_t  start_offset = 0;
  extern void* find_kernel_start32(const Elf32_Ehdr* hdr);
  extern void* find_kernel_start64(const Elf64_Ehdr* hdr);

  const char* bin_data  = nullptr;
  int         bin_len   = 0;
  char*       phys_base = nullptr;

  if (hdr->e_ident[EI_CLASS] == ELFCLASS32)
  {
    /// note: this assumes section headers are at the end
    expected_total =
        hdr->e_shnum * hdr->e_shentsize +
        hdr->e_shoff;
    /// program entry point
    void* start = find_kernel_start32(hdr);
    start_offset = (start) ? (uintptr_t) start : hdr->e_entry;
    found_kernel_start = (start != nullptr);

    // get offsets for the new service from program header
    auto* phdr = (Elf32_Phdr*) &binary[hdr->e_phoff];
    bin_data  = &binary[phdr->p_offset];
    bin_len   = phdr->p_filesz;
    phys_base = (char*) (uintptr_t) phdr->p_paddr;
  }
  else {
    auto* ehdr = (Elf64_Ehdr*) hdr;
    /// note: this assumes section headers are at the end
    expected_total =
        ehdr->e_shnum * ehdr->e_shentsize +
        ehdr->e_shoff;
    /// program entry point
    void* start = find_kernel_start64(ehdr);
    start_offset = (start) ? (uintptr_t) start : ehdr->e_entry;
    found_kernel_start = (start != nullptr);
    // get offsets for the new service from program header
    auto* phdr = (Elf64_Phdr*) &binary[ehdr->e_phoff];
    bin_data  = &binary[phdr->p_offset];
    bin_len   = phdr->p_filesz;
    phys_base = (char*) phdr->p_paddr;
  }

  if (blob.size() < expected_total || expected_total < ELF_MINIMUM)
  {
    fprintf(stderr,
        "*** There was a mismatch between blob length and expected ELF file size:\n");
    fprintf(stderr,
        "EXPECTED: %u byte\n",  (uint32_t) expected_total);
    fprintf(stderr,
        "ACTUAL:   %u bytes\n", (uint32_t) blob.size());
    throw std::runtime_error("ELF file was incomplete");
  }
  LPRINT("* Validated ELF header\n");

  // _start() entry point
  LPRINT("* Kernel entry is located at %#x\n", start_offset);

  // save ourselves if function passed
  update_store_data(storage_area, &blob);

#if !defined(PLATFORM_UNITTEST) && !defined(USERSPACE_KERNEL)
  // 2. flush all NICs
  for(auto& nic : os::machine().get<hw::Nic>())
    nic.get().flush();
  // 3. deactivate all devices (eg. mask all MSI-X vectors)
  // NOTE: there are some nasty side effects from calling this
  os::machine().deactivate_devices();
#endif
  // turn off devices that affect memory
  __arch_system_deactivate();

  // store soft-resetting stuff
#if defined(__includeos__)
  extern const std::pair<const char*, size_t> get_rollback_location();
  const auto rollback = get_rollback_location();
  // we should store physical address of update location
  auto rb_phys = os::mem::virt_to_phys((uintptr_t) rollback.first);
  void* sr_data = __os_store_soft_reset((void*) rb_phys, rollback.second);
#else
  void* sr_data = nullptr;
#endif

  // get offsets for the new service from program header
  if (bin_data == nullptr ||
      phys_base == nullptr || bin_len <= 64) {
    throw std::runtime_error("ELF program header malformed");
  }
  LPRINT("* Physical base address is %p...\n", phys_base);

  // replace ourselves and reset by jumping to _start
  LPRINT("* Replacing self with %d bytes and jumping to %#x\n", bin_len, start_offset);

#ifdef PLATFORM_x86_solo5
  solo5_exec(blob.data(), blob.size());
  throw std::runtime_error("solo5_exec returned");
# elif defined(PLATFORM_UNITTEST)
  throw liveupdate_exec_success();
# elif defined(USERSPACE_KERNEL)
  hotswap(phys_base, bin_data, bin_len, (void*) (uintptr_t) start_offset, sr_data);
  throw liveupdate_exec_success();
# elif defined(ARCH_x86_64)
    // change to simple pagetable
    __x86_init_paging((void*) 0x1000);
  if (found_kernel_start == false)
  {
    // copy hotswapping function to sweet spot
    memcpy(HOTSWAP_AREA, (void*) &hotswap64, hotswap64_len);
    /// the end
  if (LIVEUPDATE_ZERO_OLD_MEMORY) {
    ((decltype(&hotswap64)) HOTSWAP_AREA)(phys_base, bin_data, bin_len,
                start_offset,          /* binary entry point */
                sr_data,               /* softreset location */
                (void*) kernel::heap_end() /* zero memory until this location */);
  } else {
    ((decltype(&hotswap64)) HOTSWAP_AREA)(phys_base, bin_data, bin_len, start_offset, sr_data, nullptr);
  }
  }
# endif
  // copy hotswapping function to sweet spot
  memcpy(HOTSWAP_AREA, (void*) &hotswap, &__hotswap_length - (char*) &hotswap);
  /// the end
  ((decltype(&hotswap)) HOTSWAP_AREA)(phys_base, bin_data, bin_len, (void*) start_offset, sr_data);
}
void LiveUpdate::restore_environment()
{
#if defined(ARCH_x86) && !defined(PLATFORM_UNITTEST)
  // enable interrupts again
  asm volatile("sti");
#endif
}
buffer_t LiveUpdate::store()
{
  char* location = (char*) kernel::liveupdate_storage_area();
  size_t size = update_store_data(location, nullptr);
  return buffer_t(location, location + size);
}

size_t LiveUpdate::stored_data_length(const void* location)
{
  if (location == nullptr) location = kernel::liveupdate_storage_area();
  auto* storage = (storage_header*) location;

  if (storage->validate() == false)
      throw std::runtime_error("Failed sanity check on LiveUpdate storage area");

  /// return length of the whole area
  return storage->total_bytes();
}

size_t update_store_data(void* location, const buffer_t* blob)
{
  // create storage header in the fixed location
  new (location) storage_header();
  auto* storage = (storage_header*) location;

  Storage wrapper(*storage);
  /// callback for storing stuff, if provided
  for (const auto& pair : storage_callbacks)
  {
    // create partition
    int p = storage->create_partition(pair.first);
    // run serialization process
    pair.second(wrapper, blob);
    // add end for partition
    storage->finish_partition(p);
  }

  /// finalize
  storage->finalize();

  /// return length (and perform sanity check)
  return LiveUpdate::stored_data_length(location);
}

/// struct Storage

void Storage::put_marker(uid id)
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
void Storage::add_buffer(uid id, const buffer_t& blob)
{
  hdr.add_buffer(id, blob.data(), blob.size());
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
#include <net/stream.hpp>
void Storage::add_stream(net::Stream& stream)
{
  auto* stream_ptr = (net::Stream*) &stream;
  // get the sub-ID for this stream:
  // it will be used when deserializing to make sure we arent
  // calling the wrong deserialization function on the stream
  const uint16_t subid = stream_ptr->serialization_subid();
  assert(subid != 0 && "Stream should not return 0 for subid");
  // serialize the stream
  hdr.add_struct(TYPE_STREAM, subid,
  [stream_ptr] (char* location) -> int {
    // returns size of all the serialized data
    return stream_ptr->serialize_to(location, 0xFFFFFFFF);
  });
}
