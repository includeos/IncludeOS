#include <fs/vfs.hpp>

/**
 * In-memory filesystem VFS
 * i------------------------------------------i
 * | Superblock | Inode table | bitmap | data |
 * i------------------------------------------i
 * 
 * 
**/

namespace fs
{
  VFS::VFS(unsigned entries, unsigned blocks)
  {
    int nb_chunks = 1 + entries / MemBitmap::CHUNK_SIZE;
    int db_chunks = 1 + blocks  / MemBitmap::CHUNK_SIZE;
    
    // allocate entire filesystem on heap
    int total = sizeof(Superblock)
          + entries * sizeof(Inode)
          + nb_chunks * sizeof(MemBitmap::CHUNK_SIZE)
          + db_chunks * sizeof(MemBitmap::CHUNK_SIZE)
          + blocks  * BLOCK_SIZE;
    
    filesystem = new char[total];
    
    int total_notdata = total - blocks * BLOCK_SIZE;
    
    // inode table
    nodetable = (Inode*) (filesystem + sizeof(Superblock));
    // bitmap for tracking free inodes
    new (&node_bitmap) MemBitmap(nodetable + entries, nb_chunks);
    // bitmap for tracking free data segments
    new (&data_bitmap) MemBitmap(node_bitmap.location() + node_bitmap.size_bytes(), db_chunks);
    // filesystem data pointer
    diskdata = filesystem + total_notdata;
    
    // SSE zero-initialize all sections except data
    streamset8(filesystem, 0, total_notdata);
    
    // initialize superblock
    Superblock* super = superblock();
    super->entries  = entries;
    super->blocks   = blocks;
    super->root_offset = entries * sizeof(Inode);
    
    // set invalid index 0 and ROOT_ID
    node_bitmap.set(0);
    node_bitmap.set(ROOT_ID);
    
    // create root as new empty directory
    create_directory(ROOT_ID);
  }
  VFS::~VFS()
  {
    delete[] filesystem;
  }
  
  int VFS::close(inode_t file)
  {
    return -EINVAL;
  }
  
  inode_t VFS::open(const std::string& name, int flags, ...)
  {
    return -EINVAL;
  }
  
  int VFS::read(inode_t file, const std::string& ptr, int len)
  {
    return -EINVAL;
  }
  
  int VFS::write(inode_t file, const std::string& ptr, int len)
  {
    return -EINVAL;
  }
  
  inode_t VFS::symlink(const std::string& from, const std::string& to)
  {
    return 0;
  }
  int VFS::stat(const std::string& path, void* buf)
  {
    return -EINVAL;
  }
  
  int VFS::readdir(const std::string& path, void* ptr)
  {
    return -EINVAL;
  }
  int VFS::rmdir(const std::string& path)
  {
    return -EINVAL;
  }
  int VFS::mkdir(const std::string& path)
  {
    return -EINVAL;
  }
  
  VFS::Inode* VFS::get(inode_t index)
  {
    if (index < ROOT_ID) return nullptr;
    return nodetable + index;
  }
  
  // returns the first free node, or -1 on failure
  inode_t VFS::free_node() const
  {
    return node_bitmap.first_free();
  }
  
  char* VFS::getseg(int seg)
  {
    return diskdata + BLOCK_SIZE * seg;
  }
  
  int VFS::free_segment() const
  {
    return data_bitmap.first_free();
  }
  int VFS::create_segment()
  {
    int seg = free_segment();
    if (seg < 0) return -ENOSPC;
    // mark segment as used
    data_bitmap.set(seg);
    // return new empty segment
    return seg;
  }
  
  
  int VFS::create_directory(inode_t index)
  {
    Inode* n = get(index);
    if (n == nullptr) return -ENOENT;
    
    n->id   = index;
    n->type = Inode::DIR;
    n->hardlinks = 0;
    n->size      = 0;
    
    for (int i = 0; i < Inode::POINTERS; i++)
      n->data[i] = 0;
    
    return 0;
  }
  
  int VFS::directory_add(inode_t node, inode_t child)
  {
    Inode* n = get(node);
    if (n == nullptr)
      return -ENOENT;
    
    if (!n->isDir())
      return -ENOTDIR;
    
    // increase hardlinks
    n->hardlinks++;
    // 
    for (int i = 0; i < Inode::POINTERS; i++)
    {
      // FIXME: the last data pointer should be an indirection
      // if the data slot is unused, allocate a new data segment
      if (n->data[i] == 0)
      {
        // create data segment
        int seg = create_segment();
        if (seg < 0) return seg;
        // set and initialize as directory
        n->data[i] = seg;
        new (getseg(seg)) VFSdir();
      }
      // since we are here, a directory must exist
      VFSdir* dir = (VFSdir*) getseg(n->data[i]);
      // find free link
      for (int L = 0; L < VFSdir::LINKS; L++)
      {
        if (dir->nodes[L] == 0)
        {
          // add directory in free link slot
          dir->nodes[L] = child;
          return 0;
        }
      }
    } // next data pointer
    
    // we couldn't allocate data for the directory
    return -ENOSPC;
  }
  
}
