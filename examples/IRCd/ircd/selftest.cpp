#include "ircsplit.hpp"
#include <cassert>

void selftest()
{
  auto vec = ircsplit(":");
  assert(vec.size() == 1 && vec[0] == ":");

  vec = ircsplit("A");
  assert(vec.size() == 1 && vec[0] == "A");

  vec = ircsplit("ABC");
  assert(vec.size() == 1 && vec[0] == "ABC");

  vec = ircsplit("A B C");
  assert(vec.size() == 3 && vec[0] == "A" && vec[1] == "B" && vec[2] == "C");

  // second parameter can be long
  vec = ircsplit("A :B C D");
  assert(vec.size() == 2 && vec[0] == "A" && vec[1] == "B C D");

  // first parameter can't be long
  vec = ircsplit(":A B C D");
  assert(vec.size() == 4 && vec[0] == ":A");
}
