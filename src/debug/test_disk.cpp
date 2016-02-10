#include <os>
//#include <virtio/virtio_blk.hpp>

const char* service_name__ = "...";

#include <fs/disk.hpp>    // the device
#include <fs/memdisk.hpp> // the driver
#include <fs/fat.hpp>     // the filesystem
using namespace fs;

void Service::start()
{
  // mount FAT32 on partition 0 (MBR)
  using MountedDisk = fs::Disk<0, FAT32>;
  auto device = std::make_shared<MemDisk> ();
  auto disk   = std::make_shared<MountedDisk> (device);
  
  // mount the partition described by the Master Boot Record
  disk->fs().mount(MountedDisk::PART_MBR,
  [disk] (bool good)
  {
    if (!good)
    {
      printf("Could not mount filesystem\n");
      return;
    }
    
    disk->fs().ls("/",
    [disk] (bool good, FileSystem::dirvec_t ents)
    {
      if (!good)
      {
        printf("Could not list root directory");
        return;
      }
      
      for (auto& e : *ents)
      {
        printf("%s: %s\t of size %llu bytes (CL: %llu)\n",
          e.type_string().c_str(), e.name.c_str(), e.size, e.block);
        
        printf("--> %s\n", e.type_string().c_str());
        
        if (e.type == FileSystem::FILE)
        {
          printf("*** Attempting to read: %s\n", e.name.c_str());
          disk->fs().readFile(e,
          [e] (bool good, const uint8_t* buffer, size_t len)
          {
            if (!good)
            {
              printf("Failed to read file!\n");
              return;
            }
            
            std::string contents((const char*) buffer, len);
            printf("File contents:\n%s\nEOF\n\n", contents.c_str());
          });
        }
      }
    });
    
    disk->fs().stat("/test",
    [] (bool good, const FileSystem::Dirent& e)
    {
      if (!good)
      {
        printf("Could not stat the directory /test\n");
        return;
      }
      
      printf("stat: /test is a %s on cluster %llu\n", 
          e.type_string().c_str(), e.block);
      
    });
    
  });
  
  printf("*** TEST SERVICE STARTED *** \n");
}
