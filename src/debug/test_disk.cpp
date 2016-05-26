#include <os>
#include <cassert>
const char* service_name__ = "...";

#include <fs/disk.hpp>
fs::Disk_ptr disk;

#include <net/inet4>
std::unique_ptr<net::Inet4<VirtioNet> > inet;

#include <vga>
ConsoleVGA vga;

void list_partitions(decltype(disk));

void print_shit(fs::Disk_ptr disk)
{
  static size_t ints = 0;
  printf("PIT Interrupt #%u\n", ++ints);
  
  disk->dev().read(0,
  [disk] (fs::buffer_t buffer)
  {
    assert(!!buffer);
    hw::PIT::on_timeout(0.25, [disk] { print_shit(disk); });
  });
}

#include <hw/apic.hpp>
#include <hw/kbm.hpp>

void Service::start()
{
  OS::set_rsprint(
  [] (const char* data, size_t len)
  {
    vga.write(data, len);
  });
  
  // instantiate memdisk with FAT filesystem
  auto& device = hw::Dev::disk<1, VirtioBlk>();
  disk = std::make_shared<fs::Disk> (device);
  assert(disk);
  
  // boilerplate
  hw::Nic<VirtioNet>& eth0 = hw::Dev::eth<0,VirtioNet>();
  inet = std::make_unique<net::Inet4<VirtioNet> >(eth0, 0.25);
  inet->network_config(
    { 10,0,0,42 },      // IP
    { 255,255,255,0 },  // Netmask
    { 10,0,0,1 },       // Gateway
    { 8,8,8,8 } );      // DNS
  
  // if the disk is empty, we can't mount a filesystem anyways
  if (disk->empty()) panic("Oops! The disk is empty!\n");
  
  // 1. create alot of separate jobs
  for (int i = 0; i < 256; i++)
  device.read(0,
  [i] (fs::buffer_t buffer)
  {
    if (!buffer)
      printf("buffer %d is not null: %d\n", i, !!buffer);
    assert(buffer);
  });
  // 2. create alot of sequential jobs of 1024 sectors each
  // note: if we queue more than this we will run out of RAM
  static int bufcounter = 0;
  
  for (int i = 0; i < 256; i++)
  device.read(0, 22,
  [i] (fs::buffer_t buffer)
  {
    assert(buffer);
    bufcounter++;
    
    if (bufcounter == 256)
      printf("Success: All big buffers accounted for\n");
  });
  
  // list extended partitions
  list_partitions(disk);

  // mount first valid partition (auto-detect and mount)
  disk->mount(
  [] (fs::error_t err) {
    if (err) {
      printf("Error: %s\n", err.to_string().c_str());
      return;
    }
    printf("Mounted filesystem\n");
    
    // read something that doesn't exist
    disk->fs().stat("/fefe/fefe",
    [] (fs::error_t err, const fs::Dirent&) {
      printf("Error: %s\n", err.to_string().c_str());
    });
    
    // async ls
    disk->fs().ls("/",
    [] (fs::error_t err, auto ents) {
      if (err) {
        printf("Could not list '/' directory\n");
        return;
      }
      printf("Listed filesystem root\n");
      
      // go through directory entries
      for (auto& e : *ents) {
        printf("%s: %s\t of size %llu bytes (CL: %llu)\n",
               e.type_string().c_str(), e.name().c_str(), e.size(), e.block);
        
        if (e.is_file()) {
          printf("*** Read file  %s\n", e.name().c_str());
          disk->fs().read(e, 0, e.size(),
          [e] (fs::error_t err, fs::buffer_t buffer, size_t len) {
            if (err) {
              printf("Failed to read file %s!\n",
                     e.name().c_str());
              return;
            }
            
            std::string contents((const char*) buffer.get(), len);
            printf("[%s contents]:\n%s\nEOF\n\n",
                   e.name().c_str(), contents.c_str());
          });
        }
      }
    }); // ls
  }); // disk->auto_detect()
  
  //return;
  //hw::PIT::on_timeout(0.25, [] { print_shit(disk); });
  hw::KBM::init();
  
  return;
  static int job = 0;
  
  for (int i = 0; i < 10; i++)
  hw::APIC::start_task(
  [] (int cpu_id) {
    (void) cpu_id;
    __sync_fetch_and_add(&job, 1);
  }, [] {
    printf("All jobs are done now, job = %d\n", job);
  });
  
  printf("*** TEST SERVICE STARTED *** \n");
}

void list_partitions(decltype(disk) disk)
{
  disk->partitions(
  [] (fs::error_t err, auto& parts) {
    if (err) {
      printf("Failed to retrieve volumes on disk\n");
      return;
    }
    
    for (auto& part : parts)
      printf("[Partition]  '%s' at LBA %u\n",
             part.name().c_str(), part.lba());
  });
}
