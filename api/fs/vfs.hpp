// Part of the IncludeOS Unikernel  - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and  Alfred Bratterud. 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#ifndef FS_VFS_HPP
#define FS_VFS_HPP

#include <stdint.h>
#include <string>
#include <deque>
#include <membitmap>
#include <memstream>
#include "fs.hpp"
#include "path.hpp"

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
    static const int POINTERS   = 4;
    static const int BLOCK_SIZE = 4096;
    
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
    
    int mapping(int filepos, 
        int* dbl_indirect_idx, int* indirect_idx, int* block_offset)
    {
      *dbl_indirect_idx = 0;
      *indirect_idx = 0;
      
      if (filepos >= (dsize + indsize))
      {
        // double-indirect blocks
        filepos -= dsize + indsize;
        *dbl_indirect_idx = filepos / indsize;
      }
      if (filepos >= indsize)
      {
        // indirect blocks
        filepos -= *dbl_indirect_idx * indsize;
        *indirect_idx = filepos / BLOCK_SIZE;
      }
      
      // offset in data block
      filepos -= *indirect_idx * BLOCK_SIZE;
      *block_offset = filepos;
      return 0;
    }
    
    inode_t  id;
    uint8_t  type;
    uint8_t  hardlinks;
    uint32_t dsize;   // direct size
    uint32_t indsize; // indirect size
    
    int data[POINTERS];
    std::string name;
  };
  
  class VFS : public FilesystemBase
  {
  public:
    typedef VFSnode       Inode;
    typedef VFSsuperblock Superblock;
    static const int ROOT_ID     =  1;
    static const int BLOCK_SIZE  =  VFSnode::BLOCK_SIZE;
    static const int UNUSED_NODE =  0;
    static const int UNUSED_SEG  = -1;
    
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
    inline Superblock* superblock()
    {
      return (Superblock*) filesystem;
    }
    inline static bool validate(inode_t index)
    {
      return index >= ROOT_ID;
    }
    inline static bool validate_seg(int seg)
    {
      return seg >= 0;
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
    
    int init_directory(inode_t node, const std::string& name);
    int directory_add(inode_t node, inode_t child);
    int directory_rem(inode_t node, inode_t child);
    int directory_find(inode_t node, Inode*& result, const std::string& name);
    std::vector<inode_t> directory_list(Inode*);
    
    int walk_path(Path path, inode_t& current, std::string& stack_top);
    
    char*     filesystem;
    Inode*    nodetable;
    MemBitmap node_bitmap;
    MemBitmap data_bitmap;
    char*     diskdata;
  };
  
  // indirect data segment structure
  struct VFSindirect
  {
    static const int BLOCKS = VFS::BLOCK_SIZE / sizeof(int);
    
    VFSindirect()
    {
      streamset32(segments, 0, sizeof(segments));
    }
    
    // list of segments
    int segments[BLOCKS];
  };
  
  struct VFSdir
  {
    static const int LINKS = VFS::BLOCK_SIZE / sizeof(inode_t);
    
    // guarantee that the directory is initalized empty
    VFSdir()
    {
      streamset8(nodes, 0, VFS::BLOCK_SIZE);
      //memset(nodes, 0, VFS::BLOCK_SIZE);
    }
    
    // list of inodes as contents
    inode_t nodes[LINKS];
  };
  
  
}

#endif
