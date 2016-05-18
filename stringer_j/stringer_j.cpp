#include "stringer_j.hpp"
#include <cassert>

using namespace stringerj;

StringerJ::StringerJ() : builder()
{
  open();
}

std::string StringerJ::str() {
  close();
  auto output = builder.str();
  assert(output.size() > 2);
  return output;
}

StringerJ& StringerJ::open() {
  builder << "{";
  return *this;
}

StringerJ& StringerJ::close() {
  // go back 1
  builder.seekp(-1, builder.cur);
  builder << " }";
  return *this;
}
