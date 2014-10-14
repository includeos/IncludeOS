#include <os>
#include <net/class_udp.hpp>

int UDP::bottom(uint8_t* data, int len){
  printf("<UDP handler> Got data - clueless. Drop!\n");
  return -1;
}
