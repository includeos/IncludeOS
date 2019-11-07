
#include <os>
#include <cassert>

class global {
  static int i;
public:
  global(){
    CHECK(1,"Global constructor printing %i",++i);
  }

  void test(){
    CHECK(1,"C++ constructor finds %i instances",i);
  }

  int instances(){ return i; }

  ~global(){
    CHECK(1,"C++ destructor deleted 1 instance,  %i remains",--i);
  }

};


int global::i = 0;

global glob1;

int _test_glob2 = 1;
int _test_glob3 = 1;

__attribute__ ((constructor)) void foo(void)
{
  _test_glob3 = 0xfa7ca7;
}



void Service::start(const std::string&)
{
  INFO("Test CRT","Testing C runtime \n");

  CHECKSERT(_test_glob3 == 0xfa7ca7, "Global C constructors in service");
  CHECKSERT(_test_glob2 == 1, "Global int initialization in service");

  global* glob2 = new global();;
  glob1.test();
  CHECKSERT(glob1.instances() == 2, "Local C++ constructors in service");

  delete glob2;
  CHECKSERT(glob1.instances() == 1, "C++ destructors in service");


  INFO("Test CRT", "SUCCESS");
}
