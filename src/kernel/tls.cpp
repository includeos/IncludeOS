#include <cassert>
#include <cstdio>
#include <cstring>

extern char _TDATA_START_;
extern char _TDATA_END_;
extern char _TBSS_START_;
extern char _TBSS_END_;

namespace tls
{
  static size_t align_value(size_t size)
  {
    if (size & 15) size += 16 - (size & 15);
    return size;
  }

  size_t get_tls_size()
  {
    const auto TDATA_SIZE = &_TDATA_END_ - &_TDATA_START_;
    const auto TBSS_SIZE = &_TBSS_END_ - &_TBSS_START_;
    return align_value(TDATA_SIZE) + align_value(TBSS_SIZE);
  }

  void fill_tls_data(char* data)
  {
    const auto TDATA_SIZE = &_TDATA_END_ - &_TDATA_START_;
    const auto TBSS_SIZE = &_TBSS_END_ - &_TBSS_START_;

    // copy over APs .tdata
    char* tdata = data;
    memcpy(tdata, &_TDATA_START_, TDATA_SIZE);
    // clear APs .tbss
    char* tbss  = data + align_value(TDATA_SIZE);
    memset(tbss, 0, TBSS_SIZE);

    //printf("TLS at %p is %lu -> %lu bytes\n", data, TDATA_SIZE + TBSS_SIZE, align_value(TDATA_SIZE) + align_value(TBSS_SIZE));
    //printf("DATA at %p is %lu -> %lu bytes\n", tdata, TDATA_SIZE, align_value(TDATA_SIZE));
    //printf("TBSS at %p is %lu -> %lu bytes\n", tbss, TBSS_SIZE, align_value(TBSS_SIZE));
  }
}
