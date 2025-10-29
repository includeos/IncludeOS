#include <common.cxx>
#include <os.hpp>
#include <kernel/cpuid.hpp>

CASE("CPUID test")
{
  EXPECT(!CPUID::detect_features_str().empty());
}
