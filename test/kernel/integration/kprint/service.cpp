
#include <os>
#include "../../../../src/include/kprint"


void Service::start()
{
  INFO("service", "Testing kprint");

  kprintf("Test 2 I can print hex: 0x%x \n", 100);

  const char* format = "truncate %s\n";
  const char* str = "bla bla bla bla bla bla bla this part should be truncated ............................................................................................................................................................................ right about .. END";
  kprintf("Test 3 String size: %i\n", strlen(str));

  // Expect this to print a string 2x the size of format
  kprintf(format, str);

  // Use the simple char* kprint function to indicate success
  // (newline should be added here since it's truncated in the previous test)
  kprint("\nSUCCESS\n\n");
}
