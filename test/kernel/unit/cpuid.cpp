#include <common.cxx>
#include <kernel/os.hpp>
#include <kernel/cpuid.hpp>

CASE("CPUID test")
{
  EXPECT(!CPUID::detect_features_str().empty());
}
