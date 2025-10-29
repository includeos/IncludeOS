#include <common.cxx>
// Random addresses to test for each PML
namespace test {
  uint64_t rand64(){
    static std::mt19937_64 mt_rand(time(0));
    return mt_rand();
  }
}
