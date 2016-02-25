#include <os>
const char* service_name__ = "...";

#include <memdisk>
#include <fs/fat.hpp> // FAT32 filesystem
using namespace fs;

// assume that devices can be retrieved as refs with some kernel API
// for now, we will just create it here
MemDisk device;

// describe a disk with FAT32 mounted on partition 0 (MBR)
using MountedDisk = fs::Disk<FAT32>;
// disk with filesystem
std::unique_ptr<MountedDisk> disk;

using namespace hw; // kill me plz
#include <ide>

using namespace std::chrono;

void Service::start()
{
  printf("Service::start()\n");
  PIT::instance().onTimeout(20ms,
  [] ()
  {
    printf("20ms\n");
  });
  
  // instantiate disk with filesystem
  auto device = Dev::disk<1, VirtioBlk> ();
  disk = std::make_unique<MountedDisk> (device);
  printf("Initialized IDE disk\n");
  
  // sync read
  auto buf = device.read_sync(0);
  printf("Sync MBR read: %p\n", (void*) buf.get());
  // async read
  device.read(0,
  [] (auto buf)
  {
    printf("Async MBR read: %p\n", (void*) buf.get());
  });
  
  // list partitions
  disk->partitions(
  [] (fs::error_t err, auto& parts)
  {
    if (err)
    {
      printf("Failed to retrieve volumes on disk\n");
      return;
    }
    
    for (auto& part : parts)
    {
      printf("* Volume: %s at LBA %u\n",
          part.name().c_str(), part.lba_begin);
    }
  });
  
  // mount auto-detected partition
  disk->mount(
  [] (fs::error_t err)
  {
    if (err)
    {
      printf("Could not mount filesystem\n");
      return;
    }
    
    disk->fs().ls("/",
    [] (fs::error_t err, FileSystem::dirvec_t ents)
    {
      if (err)
      {
        printf("Could not list '/Sample Pictures' directory\n");
        return;
      }
      
      for (auto& e : *ents)
      {
        printf("%s: %s\t of size %llu bytes (CL: %llu)\n",
          e.type_string().c_str(), e.name.c_str(), e.size, e.block);
        
        if (e.type == FileSystem::FILE)
        {
          printf("*** Attempting to read: %s\n", e.name.c_str());
          disk->fs().readFile(e,
          [e] (fs::error_t err, fs::buffer_t buffer, size_t len)
          {
            if (err)
            {
              printf("Failed to read file %s!\n",
                  e.name.c_str());
              return;
            }
            
            std::string contents((const char*) buffer.get(), len);
            printf("[%s contents]:\n%s\nEOF\n\n", 
                e.name.c_str(), contents.c_str());
          });
        }
      }
    });
    
    disk->fs().stat("/test.txt",
    [] (fs::error_t err, const FileSystem::Dirent& e)
    {
      if (err)
      {
        printf("Could not stat /TEST.TXT\n");
        return;
      }
      
      printf("stat: /test.txt is a %s on cluster %llu\n", 
          e.type_string().c_str(), e.block);
    });
    disk->fs().stat("/Sample Pictures/Koala.jpg",
    [] (fs::error_t err, const FileSystem::Dirent& e)
    {
      if (err)
      {
        printf("Could not stat /Sample Pictures/Koala.jpg\n");
        return;
      }
      
      printf("stat: /Sample Pictures/Koala.jpg is a %s on cluster %llu\n", 
          e.type_string().c_str(), e.block);
    });
    
  }); // disk->auto_detect()
  
  printf("*** TEST SERVICE STARTED *** \n");
}
