#include <net/tcp/packet.hpp>
#include <common.cxx>

using namespace std::string_literals;

extern lest::tests & specification();

CASE("round_up returns number of d-sized chunks required for n")
{
  unsigned res;
  unsigned chunk_size {128};
  res = round_up(127, chunk_size);
  EXPECT(res == 1);
  res = round_up(128, chunk_size);
  EXPECT(res == 1);
  res = round_up(129, chunk_size);
  EXPECT(res == 2);
}

CASE("round_up expects n to be greater than 0")
{
  unsigned res;
  unsigned chunk_size {128};
  EXPECT_THROWS(round_up(0, chunk_size));
}
