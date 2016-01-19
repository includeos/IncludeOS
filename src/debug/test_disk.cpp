#include <os>
#include <virtio/virtio_blk.hpp>

// our VGA output module
//#include <kernel/vga.hpp>
//ConsoleVGA vga; //<-- krasj

const char* service_name__ = "...";

void Service::start()
{
  // set secondary serial output to VGA console module
  
  /*OS::set_rsprint_secondary(
  [] (const char* string, size_t len)
  {
    vga.write(string, len);
  });*/
  
  // initialize virtio-disk
  auto& disk1 = Dev::disk<1, VirtioBlk>();
  
  disk1.read(0,
  [] (uint64_t blk, const char* stuff)
  {
    size_t len = strlen(stuff);
    printf("disk1::read returns %d for block %llu\n", len, blk);
  });
  
  printf("*** TEST SERVICE STARTED *** \n");
}
