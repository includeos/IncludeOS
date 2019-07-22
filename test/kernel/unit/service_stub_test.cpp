
#include <common.cxx>
#include <os>

CASE("Service::binary_name() returns name of binary")
{
  EXPECT(Service::binary_name() == std::string("Service binary name"));
}

CASE("Service::name() returns name of service")
{
  EXPECT(Service::name() == std::string("Service name"));
}
