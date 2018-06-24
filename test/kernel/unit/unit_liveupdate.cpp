#include <common.cxx>
#include <liveupdate.hpp>
#include <elf.h>
using namespace liu;

static buffer_t not_a_kernel;
static void* storage_area = nullptr;
static struct {
  int         integer = 0;
  std::string string  = "";
  bool boolean        = false;
} stored;

static void store_something(Storage& store, const buffer_t*)
{
  store.add_int(0, 1234);
  store.add_string(1, "Test string");
  store.add<bool> (2, true);
}
static void restore_something(Restore& thing)
{
  stored.integer = thing.as_int(); thing.go_next();
  stored.string = thing.as_string(); thing.go_next();
  stored.boolean = thing.as_type<bool>(); thing.go_next();
}

CASE("Setup LiveUpdate and perform no-op update")
{
  storage_area = new char[16*1024*1024];

  not_a_kernel.resize(164);
  auto* elf = (Elf64_Ehdr*) not_a_kernel.data();
  elf->e_ident[0] = 0x7F;
  elf->e_ident[1] = 'E';
  elf->e_ident[2] = 'L';
  elf->e_ident[3] = 'F';
  elf->e_entry = 0x7F;
  elf->e_phoff = 64;
  elf->e_shnum = 0;
  elf->e_shoff = 164;

  auto* phdr = (Elf64_Phdr*) &not_a_kernel[elf->e_phoff];
  phdr->p_filesz = 164;
  phdr->p_paddr  = 0x80;

  EXPECT_THROWS_AS(LiveUpdate::exec(not_a_kernel, storage_area), liveupdate_exec_success);
}

CASE("Store some data and restore it")
{
  LiveUpdate::register_partition("test", store_something);
  EXPECT_THROWS_AS(LiveUpdate::exec(not_a_kernel, storage_area), liveupdate_exec_success);

  EXPECT(LiveUpdate::partition_exists("test", storage_area));

  LiveUpdate::resume_from_heap(storage_area, "test", restore_something);

  EXPECT(stored.integer == 1234);
  EXPECT(stored.string  == "Test string");
  EXPECT(stored.boolean == true);
}
