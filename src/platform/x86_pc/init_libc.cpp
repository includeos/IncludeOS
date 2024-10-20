#include "init_libc.hpp"

#include <arch/x86/cpu.hpp>
#include <kernel.hpp>
#include <kernel/auxvec.h>
#include <kernel/cpuid.hpp>
#include <kernel/diag.hpp>
#include <kernel/rng.hpp>
#include <kernel/service.hpp>
#include <util/elf_binary.hpp>
#include <version.h>
#include <kprint>

//#define KERN_DEBUG 1
#ifdef KERN_DEBUG
#define KDEBUG(fmt, ...) kprintf(fmt, ##__VA_ARGS__)
#else
#define KDEBUG(fmt, ...) /* fmt */
#endif

extern char _ELF_START_;
extern char _ELF_END_;
extern char _INIT_START_;
extern char _FINI_START_;
extern char _SSP_INIT_;
static uint32_t grub_magic;
static uint32_t grub_addr;

static volatile int global_ctors_ok = 0;
__attribute__((constructor))
static void global_ctor_test(){
  global_ctors_ok = 42;
}

namespace kernel::diag {
  void default_post_init_libc() noexcept {
    Expects(global_ctors_ok == 42 && "Global constructors functional");
    Elf_binary<Elf64> elf{{(char*)&_ELF_START_, static_cast<size_t>(&_ELF_END_ - &_ELF_START_)}};
    Expects(elf.is_ELF() && "ELF header intact");
  }
  void __attribute__((weak)) post_init_libc() noexcept {
    default_post_init_libc();
  }
}

extern "C"
int kernel_main(int, char * *, char * *)
{
  KDEBUG("<kernel_main> libc initialization complete \n");
  kernel::state().libc_initialized = true;
  kernel::diag::hook<kernel::diag::post_init_libc>();
  KDEBUG("<kernel_main> OS start \n");

  // Initialize early OS, platform and devices
#if defined(PLATFORM_x86_pc)
  kernel::start(grub_magic, grub_addr);
#elif defined(PLATFORM_x86_solo5)
  //kernel::start((const char*) (uintptr_t) grub_magic);
  kernel::start("Testing");
#else
  LL_ASSERT(0 && "Implement call to kernel start for this platform");
#endif

  // verify certain read-only sections in memory
  // NOTE: because of page protection we can choose to stop checking here
  kernel_sanity_checks();

  KDEBUG("<kernel_main> post start \n");
  // Initialize common subsystems and call Service::start
  kernel::post_start();

  // Starting event loop from here allows us to profile OS::start
  os::event_loop();
  return 0;
}

namespace x86
{
  // Musl entry
  extern "C"
  int __libc_start_main(int (*main)(int,char **,char **), int argc, char **argv);

  void init_libc(uint32_t magic, uint32_t addr)
  {
    grub_magic = magic;
    grub_addr  = addr;

    KDEBUG("* Elf start: %p\n", &_ELF_START_);
    auto* ehdr = (Elf64_Ehdr*)&_ELF_START_;
    auto* phdr = (Elf64_Phdr*)((char*)ehdr + ehdr->e_phoff);
    LL_ASSERT(phdr);
    Elf_binary<Elf64> elf{{(char*)&_ELF_START_, static_cast<size_t>(&_ELF_END_ - &_ELF_START_)}};
    LL_ASSERT(elf.is_ELF());
    LL_ASSERT(phdr[0].p_type == PT_LOAD);

  #ifdef KERN_DEBUG
    KDEBUG("* Elf ident: %s, program headers: %p\n", ehdr->e_ident, ehdr);
    size_t size =  &_ELF_END_ - &_ELF_START_;
    KDEBUG("\tElf size: %zu \n", size);
    for (int i = 0; i < ehdr->e_phnum; i++)
    {
      KDEBUG("\tPhdr %i @ %p, va_addr: 0x%lx \n", i, &phdr[i], phdr[i].p_vaddr);
    }
  #endif

    // Build AUX-vector for C-runtime
    std::array<char*, 6 + 38> argv;
    // Parameters to main
    argv[0] = (char*) Service::name();
    argv[1] = 0x0;
    int argc = 1;

    // Env vars
    argv[2] = std::pmr::string("LC_CTYPE=C").data();
    argv[3] = std::pmr::string("LC_ALL=C").data();
    argv[4] = std::pmr::string("USER=root").data();
    argv[5] = 0x0;

    // auxiliary vector
    auxv_t* aux = (auxv_t*) &argv[6];
    KDEBUG("* Initializing aux-vector @ %p\n", aux);

    int i = 0;
    aux[i++].set_long(AT_PAGESZ, 4096);
    aux[i++].set_long(AT_CLKTCK, 100);

    // ELF related
    aux[i++].set_long(AT_PHENT, ehdr->e_phentsize);
    aux[i++].set_ptr(AT_PHDR, ((uint8_t*)ehdr) + ehdr->e_phoff);
    aux[i++].set_long(AT_PHNUM, ehdr->e_phnum);

    // Misc
    aux[i++].set_ptr(AT_BASE, nullptr);
    aux[i++].set_long(AT_FLAGS, 0x0);
    aux[i++].set_ptr(AT_ENTRY, (void*) &kernel_main);
    aux[i++].set_long(AT_HWCAP, 0);
    aux[i++].set_long(AT_UID, 0);
    aux[i++].set_long(AT_EUID, 0);
    aux[i++].set_long(AT_GID, 0);
    aux[i++].set_long(AT_EGID, 0);
    aux[i++].set_long(AT_SECURE, 1);

    const char* plat = "x86_64";
    aux[i++].set_ptr(AT_PLATFORM, plat);

    // supplemental randomness
    const long canary = rng_extract_uint64() & 0xFFFFFFFFFFFF00FFul;
    const long canary_idx = i;
    aux[i++].set_long(AT_RANDOM, canary);
    //kprintf("* Stack protector value: %#lx\n", canary);
    // entropy slot
    aux[i++].set_ptr(AT_RANDOM, &aux[canary_idx].a_un.a_val);
    aux[i++].set_long(AT_NULL, 0);

#ifdef PLATFORM_x86_pc
    // SYSCALL instruction
  #if defined(__x86_64__)
    KDEBUG("* Initialize syscall MSR (64-bit)\n");
    uint64_t star_kernel_cs = 8ull << 32;
    uint64_t star_user_cs   = 8ull << 48;
    uint64_t star = star_kernel_cs | star_user_cs;
    x86::CPU::write_msr(IA32_STAR, star);
    x86::CPU::write_msr(IA32_LSTAR, (uintptr_t)&__syscall_entry);
  #elif defined(__i386__)
    KDEBUG("Initialize syscall intr (32-bit)\n");
    #warning Classical syscall interface missing for 32-bit
  #endif
#endif

    // GDB_ENTRY;
    KDEBUG("* Starting libc initialization\n");
    kernel::state().allow_syscalls = true;
    __libc_start_main(kernel_main, argc, argv.data());
  }
}
