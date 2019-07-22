// -*-C++-*-

__attribute__((weak))
extern void __x86_init_paging(void* pdir) {
  asm volatile ("mov %%rax, %%cr3;" :: "a" (pdir));
}
