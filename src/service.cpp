#include <os>
#include <string.h>

//#include <iostream>
//#include <functional>
#include "tests/tests.h"

static const char* static_glob="Scoped variable accessed from Lambda";

void Service::start(){
  // int* i=new int(5); //Requires operator new.

  char* local1="Local variables are created during runtime";
#ifdef TEST_H
  test_print_hdr("Lambdas");
    
  //Lambda, accessing parent scope
  [&](){
    test_print_result("Lambda accessing global static var",
		      strcmp(static_glob,
			     "Scoped variable accessed from Lambda")==0);
    test_print_result("Lambda accessing external local var",
		      strcmp(local1,
			     "Local variables are created during runtime")==0);
  }();

#endif //TESTS_H
  
  /*
    //Exceptions will NOT be working for a while (__cxa_throw, etc. missing)
  try{
    throw 256;
  }catch(int e){
    if(e==256)
      OS::rsprint("[PASS]\t Basic exceptions work");
    else
      OS::rsprint("[FAIL]\t Basic exceptions NOT working");
  }
  */

  //TODO: This works fine - should it? Should services be able to use syscalls directly?  
  //sbrk(44);
  
}
