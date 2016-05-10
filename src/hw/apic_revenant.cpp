#include <cstdio>
#include <cstdint>
//#include <atomic>

int revenant_start_counter = 0;

extern "C"
void revenant_main(int cpu)
{
  
  //printf("Revenant %u started\n", cpu);
  asm volatile("cli; hlt;");
}
