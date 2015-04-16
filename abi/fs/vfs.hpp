#ifndef FS_VFS_HPP
#define FS_VFS_HPP

#include <stdint.h>
#include <string>
#include <deque>
#include <membitmap>
#include <memstream>
#include "fs.hpp"

namespace fs
{
  struct VFSsuperblock
  {
    uint16_t entries;
    uint32_t blocks;
    uint32_t root_offset;
  };
  
  // 32-byte index node contains fs metadata
  struct VFSnode
  {
    static const int POINTERS = 4;
    
    enum type_t
    {
      DIR = 0,
      FILE,
      SYMLINK,
      
      TYPE_MASK    = 31,
      INDIRECT_BIT = 32
    };
    
    inline bool isDir() const
    {
      return (type & TYPE_MASK) == DIR;
    }
    inline bool isFile() const
    {
      return (type & TYPE_MASK) == FILE;
    }
    inline bool isSymlink() const
    {
      return (type & TYPE_MASK) == SYMLINK;
    }
    
    inode_t  id;
    uint8_t  type;
    uint8_t  hardlinks;
    uint32_t size;
    
    int data[POINTERS];
  };
  
  class VFS : public FilesystemBase
  {
  public:
    typedef VFSnode       Inode;
    typedef VFSsuperblock Superblock;
    static const int ROOT_ID    = 1;
    static const int BLOCK_SIZE = 512;
    
    VFS()
      : filesystem(nullptr)  {}
    VFS(unsigned inodes, unsigned disk_data);
    ~VFS();
    
    inode_t open(const std::string& name, int flags, ...) override;
    int read(inode_t file, const std::string& ptr, int len) override;
    int write(inode_t file, const std::string& ptr, int len) override;
    int close(inode_t file) override;
    
    inode_t symlink(const std::string& from, const std::string& to) override;
    int stat(const std::string& path, void* buf) override;
    
    int readdir(const std::string& path, void* ptr) override;
    int rmdir(const std::string& path) override;
    int mkdir(const std::string& path) override;
    
  private:
    inline Inode* root()
    {
      return nodetable + ROOT_ID;
    }
    Superblock* superblock()
    {
      return (Superblock*) filesystem;
    }
    
    // returns pointer to node at @node, or nullptr on failure
    Inode*  get(inode_t node);
    // returns the first free node, or -1 on failure
    inode_t free_node() const;
    
    // returns pointer to segment or nullptr on failure
    char* getseg(int seg);
    // returns the first free segment, or -1 on failure
    int free_segment() const;
    // mark used and return a new data segment for node
    int create_segment();
    
    int create_directory(inode_t node);
    int directory_add(inode_t node, inode_t child);
    int directory_rem(inode_t node, inode_t child);
    
    char*     filesystem;
    Inode*    nodetable;
    MemBitmap node_bitmap;
    MemBitmap data_bitmap;
    char*     diskdata;
    
    std::vector<inode_t> opened;
  };
  
  struct VFSdir
  {
    static const int LINKS = VFS::BLOCK_SIZE / sizeof(inode_t);
    
    // guarantee that the directory is initalized empty
    VFSdir()
    {
      streamset16(nodes, 0, sizeof(nodes));
    }
    
    // list of inodes as contents
    inode_t nodes[LINKS];
  };
  
  
}

#endif
