#include <common.cxx>
#include <liveupdate.hpp>
#include <elf.h>
#include <os>
#include <statman>
#include "paging.inc"
using namespace liu;

// #define DEBUG_UNIT
#ifdef DEBUG_UNIT
#define MYINFO(X,...) INFO("<test os::mem>", X, ##__VA_ARGS__)
#else
#define MYINFO(X,...)
#endif

#include <net/inet>
#include <net/interfaces>
#include <nic_mock.hpp>
static net::Inet* inet = nullptr;


static buffer_t not_a_kernel;
static void* storage_area = nullptr;
struct testable_t
{
  int value;
};
static struct {
  int               integer = 0;
  std::string       string  = "";
  bool              boolean = false;
  struct testable_t testable;
  liu::buffer_t     buffer;
  std::vector<int>  intvec1;
  std::vector<int>  intvec2;
  std::vector<std::string> strvec1;
  std::vector<std::string> strvec2;
} stored;
static std::vector<int> ivec1{2, 1};
static std::vector<int> ivec2{1, 2, 3};
static std::vector<std::string> svec1{"2", "1"};
static std::vector<std::string> svec2{"1", "2", "3"};
static struct testable_t test_struct { .value = 4 };

static void store_something(Storage& store, const buffer_t*)
{
  store.add_int(0, 1234);
  store.add_string(1, "Test string");
  store.add<bool> (2, true);
  store.add<struct testable_t> (3, test_struct);
  store.add_buffer(4, not_a_kernel);
  store.add_vector<int> (5, ivec1);
  store.add_vector<int> (5, ivec2);
  store.add_vector<std::string> (6, svec1);
  store.add_vector<std::string> (6, svec2);
  // TCP connection
  auto conn = inet->tcp().connect({{1,1,1,1}, 8080});
  store.add_connection(7, conn);
  // Statman stats
  auto& sman = Statman::get();
  sman.store(8, store);
  // storing stats twice will force merging
  sman.store(8, store);
  // End marker
  store.put_marker(9);
}
static void restore_something(Restore& thing)
{
  assert(thing.get_id() == 0);
  assert(thing.is_int());
  stored.integer = thing.as_int(); thing.go_next();
  assert(thing.get_id() == 1);
  assert(thing.is_string());
  stored.string = thing.as_string(); thing.go_next();
  assert(thing.get_id() == 2);
  assert(thing.is_buffer()); // internally they are buffers
  stored.boolean = thing.as_type<bool>(); thing.go_next();
  assert(thing.get_id() == 3);
  assert(thing.is_buffer()); // internally they are buffers
  stored.testable = thing.as_type<struct testable_t>(); thing.go_next();
  assert(thing.get_id() == 4);
  assert(thing.is_buffer());
  stored.buffer = thing.as_buffer(); thing.go_next();
  assert(thing.get_id() == 5);
  assert(thing.is_vector());
  stored.intvec1 = thing.as_vector<int>(); thing.go_next();
  stored.intvec2 = thing.as_vector<int>(); thing.go_next();
  assert(thing.get_id() == 6);
  assert(thing.is_string_vector());
  stored.strvec1 = thing.as_vector<std::string>(); thing.go_next();
  stored.strvec2 = thing.as_vector<std::string>(); thing.go_next();
  // TCP connection
  assert(thing.get_id() == 7);
  auto conn = thing.as_tcp_connection(inet->tcp()); thing.go_next();
  // Statman stats
  assert(thing.get_id() == 8);
  assert(thing.is_vector());
  auto& sman = Statman::get();
  sman.restore(thing); thing.go_next();
  sman.restore(thing); thing.go_next();
  // End marker
  assert(thing.get_id() == 9);
  assert(thing.is_marker());
  thing.pop_marker(9);
  assert(thing.is_end());
}

CASE("Setup LiveUpdate and perform no-op update")
{
  Default_paging p{};
  storage_area = new char[16*1024*1024];

  not_a_kernel.resize(164 + sizeof(Elf64_Shdr));
  auto* elf = (Elf64_Ehdr*) not_a_kernel.data();
  elf->e_ident[0] = 0x7F;
  elf->e_ident[1] = 'E';
  elf->e_ident[2] = 'L';
  elf->e_ident[3] = 'F';
  elf->e_entry = 0x7F;
  elf->e_phoff = 64;
  elf->e_shnum = 1;
  elf->e_shoff = 164;
  // this will trigger more elfscan code
  auto* shdr = (Elf64_Shdr*) &not_a_kernel[164];
  shdr->sh_type = SHT_SYMTAB;
  shdr->sh_size = 0;

  auto* phdr = (Elf64_Phdr*) &not_a_kernel[elf->e_phoff];
  phdr->p_filesz = not_a_kernel.size();
  phdr->p_paddr  = 0x80;

  EXPECT_THROWS_AS(LiveUpdate::exec(not_a_kernel, storage_area), liveupdate_exec_success);
}

CASE("Store some data and restore it")
{
  Default_paging p{};
  Nic_mock nic;
  net::Inet netw{nic};
  inet = &netw;

  LiveUpdate::register_partition("test", store_something);
  try {
    LiveUpdate::exec(not_a_kernel, storage_area);
  }
  catch (const liu::liveupdate_exec_success& e) {
    printf("LiveUpdate: %s\n", e.what());
  }
  catch (const std::exception& e) {
    printf("LiveUpdate error: %s\n", e.what());
    throw;
  }
  LiveUpdate::restore_environment();

  // clear all stats before restoring them
  Statman::get().clear();

  EXPECT_THROWS_AS(LiveUpdate::resume_from_heap(storage_area, "", nullptr), std::length_error);
  
  EXPECT(LiveUpdate::partition_exists("test", storage_area));
  LiveUpdate::resume_from_heap(storage_area, "test", restore_something);

  
  //LiveUpdate::resume("test", restore_something);

  EXPECT(stored.integer == 1234);
  EXPECT(stored.string  == "Test string");
  EXPECT(stored.boolean == true);
  EXPECT(stored.testable.value == test_struct.value);
  EXPECT(stored.buffer  == not_a_kernel);
  EXPECT(stored.intvec1 == ivec1);
  EXPECT(stored.intvec2 == ivec2);
  EXPECT(stored.strvec1 == svec1);
  EXPECT(stored.strvec2 == svec2);
}
