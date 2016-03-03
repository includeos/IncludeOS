#include <os>
const char* service_name__ = "...";

#include <memdisk>

// memdisk with FAT filesystem
fs::MountedDisk disk;

void list_partitions(fs::MountedDisk);

void Service::start()
{
  // instantiate memdisk with FAT filesystem
  disk = fs::new_shared_memdisk();
  
  // if the disk is empty, we can't mount a filesystem anyways
  if (disk->empty()) panic("Oops! The memdisk is empty!\n");
  
  // list extended partitions
  list_partitions(disk);
  
  // mount first valid partition (auto-detect and mount)
  disk->mount( // or specify partition explicitly in parameter
  [] (fs::error_t err)
  {
    if (err)
    {
      printf("Could not mount filesystem\n");
      return;
    }
    // get a reference to the mounted filesystem
    auto& fs = disk->fs();
    
    // check contents of disk
    auto dirents = fs::new_shared_vector();
    err = fs.ls("/", dirents);
    if (err)
      printf("Could not list '/' directory\n");
    else
      for (auto& e : *dirents)
      {
        printf("%s: %s\t of size %llu bytes (CL: %llu)\n",
          e.type_string().c_str(), e.name().c_str(), e.size, e.block);
      }
    
    auto ent = fs.stat("/test.txt");
    // validate the stat call
    if (ent.is_valid())
    {
      // read specific area of file
      auto buf = fs.read(ent, 1032, 65);
      std::string contents((const char*) buf.buffer.get(), buf.len);
      printf("[%s contents (%llu bytes)]:\n%s\n[end]\n\n", 
             ent.name().c_str(), buf.len, contents.c_str());
      
    }
    return;
    
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
        
        if (e.is_file())
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

void list_partitions(fs::MountedDisk disk)
{
  disk->partitions(
  [] (fs::error_t err, auto& parts)
  {
    if (err)
    {
      printf("Failed to retrieve volumes on disk\n");
      return;
    }
    
    for (auto& part : parts)
      printf("[Partition]  '%s' at LBA %u\n",
          part.name().c_str(), part.lba());
  });
}
