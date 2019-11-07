
#include <common.cxx>
#include <os.hpp>
#include <kernel/memory.hpp>

CASE("version() returns string representation of OS version")
{
  EXPECT(os::version() != nullptr);
  EXPECT(std::string(os::version()).size() > 0);
  EXPECT(os::version()[0] == 'v');
  EXPECT(os::arch() != nullptr);
  EXPECT(std::string(os::arch()).size() > 0);
}

CASE("cycles_since_boot() returns clock cycles since boot")
{
  EXPECT(os::cycles_since_boot() != 0ull);
}

CASE("page_size() returns page size")
{
  EXPECT(os::mem::min_psize() == 4096u);
}
