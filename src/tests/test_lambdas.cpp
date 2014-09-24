#include "tests.hpp"

static const char* static_glob="Scoped variable accessed from Lambda";


char* local1=(char*)"Local variables are created during runtime";

void test_lambdas(){
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
}
