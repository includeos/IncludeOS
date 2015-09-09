#include <quick_exit>
#include <stdlib.h>
#include <malloc.h>

extern "C" void panic(const char*);


void __default_quick_exit(){
  panic("\n>>> Quick exit, default route \n");
}


// According to the standard this should probably be a list or vector.
static void (*__quick_exit_func)(void) = __default_quick_exit;

int at_quick_exit (void (*func)(void)){
  // Append to the ist
  __quick_exit_func = func;
  return 0;
};



_Noreturn void quick_exit (int status){
  
  // Call the exit-function(s) and then _Exit
  __quick_exit_func();
  
  
  printf("\n>>> EXIT_%s (%i) \n",status==0 ? "SUCCESS" : "FAILURE",status);

  
  // Well. 
  panic("Quick exit called. ");
  
  // ...we could actually return to the OS. Like, if we want to stay responsive, answer ping etc.
  // How to clean up the stack? Do we even need to?
};


// Defined in memstream.c
// void *aligned_alloc( size_t alignment, size_t size ){
//   return memalign(alignment, size);
// }; 
