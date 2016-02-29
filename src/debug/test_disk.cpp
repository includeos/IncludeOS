#include <os>
const char* service_name__ = "...";

#include <memdisk>
#include <fs/fat.hpp> // FAT32 filesystem
using namespace fs;

// assume that devices can be retrieved as refs with some kernel API
// for now, we will just create it here
MemDisk device;

// describe a disk with FAT32 mounted on partition 0 (MBR)
using MountedDisk = fs::Disk<FAT>;
// disk with filesystem
std::unique_ptr<MountedDisk> disk;

using namespace hw; // kill me plz
#include <virtio/console.hpp>

using namespace std::chrono;

void Service::start()
{
  /*auto& con = Dev::console<0, VirtioCon> ();
  OS::set_rsprint(
  [&con] (const char* data, size_t len)
  {
    con.write(data, len);
  });*/
  
  // instantiate disk with filesystem
  //auto device = Dev::disk<1, VirtioBlk> ();
  disk = std::make_unique<MountedDisk> (device);
  
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
    
    auto dirents = std::make_shared<fs::dirvec_t> ();
    err = disk->fs().ls("/", dirents);
    if (err)
    {
      printf("Could not list '/' directory\n");
    }
    else
    {
      for (auto& e : *dirents)
      {
        printf("%s: %s\t of size %llu bytes (CL: %llu)\n",
          e.type_string().c_str(), e.name().c_str(), e.size, e.block);
        
        if (e.type() == FileSystem::FILE)
        {
          printf("*** Attempting to read: %s\n", e.name().c_str());
          auto buf = disk->fs().read(e, 0, e.size);
          
          if (buf.err)
          {
            printf("Failed to read file %s!\n", e.name().c_str());
          }
          else
          {
            std::string contents((const char*) buf.buffer.get(), buf.len);
            printf("[%s contents]:\n%s\nEOF\n\n", 
                e.name().c_str(), contents.c_str());
          }
        }
      }
    }
    
    disk->fs().ls("/",
    [] (fs::error_t err, auto ents)
    {
      if (err)
      {
        printf("Could not list '/' directory\n");
        return;
      }
      
      for (auto& e : *ents)
      {
        printf("%s: %s\t of size %llu bytes (CL: %llu)\n",
          e.type_string().c_str(), e.name().c_str(), e.size, e.block);
        
        if (e.type() == FileSystem::FILE)
        {
          printf("*** Attempting to read: %s\n", e.name().c_str());
          disk->fs().readFile(e,
          [e] (fs::error_t err, fs::buffer_t buffer, size_t len)
          {
            if (err)
            {
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
    });
    
    disk->fs().stat("/test.txt",
    [] (fs::error_t err, const auto& e)
    {
      if (err)
      {
        printf("Could not stat %s\n", e.name().c_str());
        return;
      }
      
      printf("stat: /test.txt is a %s on cluster %llu\n", 
          e.type_string().c_str(), e.block);
    });
    disk->fs().stat("/Sample Pictures/Koala.jpg",
    [] (fs::error_t err, const auto& e)
    {
      if (err)
      {
        printf("Could not stat %s\n", e.name().c_str());
        return;
      }
      
      printf("stat: %s is a %s on cluster %llu\n", 
          e.name().c_str(), e.type_string().c_str(), e.block);
    });
    
  }); // disk->auto_detect()
  
  printf("*** TEST SERVICE STARTED *** \n");
}
