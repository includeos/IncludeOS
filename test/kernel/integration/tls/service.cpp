
#include <service>
#include <cassert>

// TBSS area
thread_local int test_int = 0;
thread_local char test_char = 0;
// TDATA area
thread_local char test_array[3] = {1, 2, 3};
thread_local int64_t test_i64 = 0x11ABCDEF22ABCDEF;

void Service::start()
{
  int bss_local = 0;
  int data_local = 1;
  // TBSS area
  assert(test_int == 0);
  assert(test_char == 0);
  // modify TBSS
  test_int = 1;
  assert(test_int == 1);
  // TDATA area
  assert(test_array[0] == 1);
  assert(test_array[1] == 2);
  assert(test_array[2] == 3);
  assert(test_i64 == 0x11ABCDEF22ABCDEF);
  // modify TDATA area
  test_array[0] = 44;
  assert(test_array[0] == 44);
  assert(test_array[1] == 2);
  assert(test_array[2] == 3);
  // verify locals
  assert(bss_local == 0);
  assert(data_local == 1);
  printf("SUCCESS\n");
}
