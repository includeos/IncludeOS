#include "class_os.h"

int main();

extern "C" {
  void _init();
}
extern "C" {
  void _start(void){    
    OS::rsprint(" \n\n *** IncludeOS Initializing *** \n\n");    

    //Initialize .bss secion (It's garbage in qemu)
    OS::rsprint(">>> Initializing .bss... \n");
    
    
    char* bss=&_BSS_START_;
    *bss=0;
    while(++bss < &_BSS_END_)
      *bss=0;
  
    test_print_hdr("Global constructors");
    //Call global constructors (relying on .crtbegin to be inserted by gcc)
    _init();

    
    OS::rsprint("\n>>> IncludeOS Initialized. Calling main\n");
    
    main();
    
    //Will only work if any destructors are called (I think?)
    //    _fini();

  }

  int main(){
    OS::start();
    //OBS: If this function returns, the consequences are UNDEFINED
    return 0;
  }
  
}


