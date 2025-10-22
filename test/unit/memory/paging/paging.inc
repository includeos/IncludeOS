#pragma once
#include <arch/x86/paging.hpp>
#include <info>

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
    INFO("Test", "Initializing default paging");
    __arch_init_paging();
  }


  static void clear_paging() {
    using namespace x86::paging;
    INFO("Test", "Clearing default paging");
    if (__pml4 != nullptr) {
      __pml4->~Pml4();
      free(__pml4);
      __pml4 = nullptr;
      os::mem::vmmap().clear();
    }
  }
};
