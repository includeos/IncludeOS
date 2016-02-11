#include <fs/ext4.hpp>

#include <cassert>
#include <fs/mbr.hpp>

namespace fs
{
  EXT4::EXT4(IDiskDevice& dev)
    : device(dev)
  {
    
  }
  
  void EXT4::init(const void* base_sector)
  {
    (void) base_sector;
  }
  
  void EXT4::mount(uint8_t partid, on_mount_func on_mount)
  {
    printf("Superblock: %u bytes, Block group desc: %u bytes\n", 
        sizeof(superblock), sizeof(group_desc));
    assert(sizeof(superblock) == 1024);
    assert(sizeof(group_desc) == 64);
    
    printf("Inode table: %u\n",
        sizeof(inode_table));
    
    // read Master Boot Record (sector 0)
    device.read_sector(0,
    [this, partid, on_mount] (const void* data)
    {
      auto* mbr = (MBR::mbr*) data;
      assert(mbr != nullptr);
      
      /// now what?
      
    });
    
  }
  
  void EXT4::ls(const std::string& path, on_ls_func on_ls)
  {
    
  }
  
  void EXT4::readFile(const Dirent& ent, on_read_func callback)
  {
    callback(true, nullptr, 0);
  }
  void EXT4::readFile(const std::string& strpath, on_read_func callback)
  {
    callback(true, nullptr, 0);
  }
  
  void EXT4::stat(const std::string& strpath, on_stat_func callback)
  {
    callback(true, Dirent());
  }
  
  // filesystem traversal function
  void EXT4::traverse(std::shared_ptr<Path> path, cluster_func callback)
  {
    
  }
  
}
