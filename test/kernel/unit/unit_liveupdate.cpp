#include <common.cxx>
#include <liveupdate.hpp>
#include <arch/x86/paging.hpp>
#include <elf.h>
#include <os>
using namespace liu;

// #define DEBUG_UNIT
#ifdef DEBUG_UNIT
#define MYINFO(X,...) INFO("<test os::mem>", X, ##__VA_ARGS__)
#else
#define MYINFO(X,...)
#endif

static buffer_t not_a_kernel;
static void* storage_area = nullptr;
static struct {
  int         integer = 0;
  std::string string  = "";
  bool boolean        = false;
} stored;

extern void  __arch_init_paging();
extern x86::paging::Pml4* __pml4;
// Default page setup RAII
class Default_paging {
public:
  ~Default_paging()
  {
    clear_paging();
  }

  Default_paging()
  {
    clear_paging();
    MYINFO("Initializing default paging \n");
    __arch_init_paging();
  }


  static void clear_paging() {
    using namespace x86::paging;
    MYINFO("Clearing default paging \n");
    if (__pml4 != nullptr) {
      __pml4->~Pml4();
      free(__pml4);
      __pml4 = nullptr;
      OS::memory_map().clear();
    }
  }
};
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
  Default_paging p{};
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
  phdr->p_filesz = not_a_kernel.size();
  phdr->p_paddr  = 0x80;

  EXPECT_THROWS_AS(LiveUpdate::exec(not_a_kernel, storage_area), liveupdate_exec_success);
}

CASE("Store some data and restore it")
{
  Default_paging p{};
  LiveUpdate::register_partition("test", store_something);
  EXPECT_THROWS_AS(LiveUpdate::exec(not_a_kernel, storage_area), liveupdate_exec_success);

  EXPECT(LiveUpdate::partition_exists("test", storage_area));

  LiveUpdate::resume_from_heap(storage_area, "test", restore_something);

  EXPECT(stored.integer == 1234);
  EXPECT(stored.string  == "Test string");
  EXPECT(stored.boolean == true);
}
