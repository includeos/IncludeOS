#include <cstdint>
#include <cassert>
#include <cstdio>
#include <malloc.h>
#include <vector>

extern char _TDATA_START_;
extern char _TDATA_END_;
extern char _TBSS_START_;
extern char _TBSS_END_;

namespace tls
{
  static std::vector<char*> per_cpu_data;

  char* get_tls_data(int cpu)
  {
    return per_cpu_data.at(cpu);
  }

  void init(const int num_CPUs)
  {
    auto TDATA_SIZE = &_TDATA_END_ - &_TDATA_START_;
    auto TBSS_SIZE = &_TDATA_END_ - &_TDATA_START_;
    auto TLS_TOTAL = TDATA_SIZE + TBSS_SIZE;

    // initialize .tdata and .tbss for each CPU
    for (int cpu = 0; cpu < num_CPUs; cpu++)
    {
      per_cpu_data.push_back((char*) memalign(64, TLS_TOTAL));
      //printf("CPU %d TLS at %p is %lu bytes\n", cpu, per_cpu_data.back(), TLS_TOTAL);
      // copy over APs .tdata
      memcpy(per_cpu_data.back(), &_TDATA_START_, TDATA_SIZE);
      // clear APs .tbss
      memset(per_cpu_data.back() + TDATA_SIZE, 0, TBSS_SIZE);
    }

  }

}
