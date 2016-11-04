
#include <assert.h>

extern "C" {

  void panic(const char* why){

    assert(0 && "Panic: " && why);
  }

}
