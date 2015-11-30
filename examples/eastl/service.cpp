#include <os>

#include "test.hpp"

void Service::start()
{
  std::cout << "*** Service is up - with OS Included! ***" << std::endl;
  
  TestResult results;
  
  results.emplace_back("Test 1", 1, 1, "This test does nothing");
  
  for (auto& test : results)
  {
    std::cout << test.report() << std::endl;
  }
  
  // do the STREAM test here
  //int tests();
  
  std::cout << "Service out!" << std::endl;
}
