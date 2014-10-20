#include <os>
#include <net/class_icmp.hpp>

int ICMP::bottom(uint8_t* data, int len){
  printf("<ICMP handler> Got data - clueless. Drop!\n");
  return -1;
}
