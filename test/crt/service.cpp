#include <os>

class global {
  static int i;
public:
  global(){
    printf("[*] Global constructor printing %i \n",++i);
  }
  
  void test(){
    printf("[*] C++ constructor finds %i instances \n",i);
  }
  
  int instances(){ return i; }
  
  ~global(){
    printf("[*] C++ destructor deleted 1 instance,  %i remains \n",--i);
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



void Service::start()
{
  
  printf("TESTING C runtime \n");

  printf("[%s] Global C constructors in service \n", 
         _test_glob3 == 0xfa7ca7 ? "x" : " ");
  
  printf("[%s] Global int initialization in service \n", 
         _test_glob2 == 1 ? "x" : " ");
  
  
  global* glob2 = new global();;
  glob1.test();
  printf("[%s] Local C++ constructors in service \n", glob1.instances() == 2 ? "x" : " ");

  
  delete glob2;
  printf("[%s] C++ destructors in service \n", glob1.instances() == 1 ? "x" : " ");
  
  
  
  
}
