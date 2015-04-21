#include <fs/vfs.hpp>
#include <fs/path.hpp>

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
    
    filesystem = (char*) aligned_alloc(total, SSE_SIZE);
    
    int total_notdata = total - blocks * BLOCK_SIZE;
    
    // inode table
    nodetable = (Inode*) (filesystem + sizeof(Superblock));
    // bitmap for tracking free inodes
    new (&node_bitmap) MemBitmap(nodetable + entries, nb_chunks);
    // bitmap for tracking free data segments
    new (&data_bitmap) MemBitmap(node_bitmap.data() + node_bitmap.size(), db_chunks);
    // filesystem data pointer
    diskdata = filesystem + total_notdata;
    
    // SSE zero-initialize all sections except data
    streamset8(filesystem, 0, total_notdata);
    
    // initialize superblock
    Superblock* super = superblock();
    super->entries  = entries;
    super->blocks   = blocks;
    super->root_offset = entries * sizeof(Inode);
    
    // set invalid index 0 as used
    node_bitmap.set(0);
    
    // initialize root as new empty directory
    std::cout << ">>> VFS: initializing root" << std::endl;
    init_directory(ROOT_ID, "");
    std::cout << ">>> VFS: done" << std::endl;
  }
  VFS::~VFS()
  {
    aligned_free(filesystem);
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
    Inode*      node = nullptr;
    inode_t     current;
    std::string name;
    
    std::cout << "rmdir: path = " << path << std::endl;
    
    int res = walk_path(path, current, name);
    if (res)
    {
      std::cout << ">>> VFS::rmdir() walk_path result (" << res << "): " << fs_error_string(res) << std::endl;
      return res;
    }
    
    std::cout << "rmdir: current = " << current << " name = " << name << std::endl;
    
    // find Inode
    res = directory_find(current, node, name);
    if (res)
    {
      std::cout << ">>> VFS::rmdir() directory_find result (" << res << "): " << fs_error_string(res) << std::endl;
      return res;
    }
    std::cout << "rmdir: removing " << node->id << " from " << current << std::endl;
    res = directory_rem(current, node->id);
    if (res)
    {
      std::cout << ">>> VFS::rmdir() directory_rem result (" << res << "): " << fs_error_string(res) << std::endl;
      return res;
    }
    
    return 0;
  }
  int VFS::mkdir(const std::string& path)
  {
    inode_t     current;
    std::string new_name;
    
    int res = walk_path(path, current, new_name);
    if (res)
    {
      std::cout << ">>> VFS::mkdir() walk_path result (" << res << "): " << fs_error_string(res) << std::endl;
      return res;
    }
    
    Inode* cnode = nullptr;
    
    // check for duplicate entry in directory
    directory_find(current, cnode, new_name);
    if (cnode)
    {
      std::cout << ">>> VFS::mkdir() file already exists (" << new_name << ")" << std::endl;
      return -EEXIST;
    }
    
    // create new directory
    inode_t newnode = free_node();
    if (!validate(newnode))
    {
      std::cout << ">>> VFS::mkdir() could not create new node (" << newnode << ")" << std::endl;
      return -ENOSPC;
    }
    init_directory(newnode, new_name);
    
    // add directory to current
    std::cout << "****** ADDING " << newnode << " TO " << current << std::endl;
    directory_add(current, newnode);
    return 0;
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
    if (seg < 0)
    {
      std::cout << ">>> VFS::create_segment() no free space" << std::endl;
      return -ENOSPC;
    }
    // mark segment as used
    data_bitmap.set(seg);
    // return new empty segment
    return seg;
  }
  
  
  int VFS::init_directory(inode_t index, const std::string& name)
  {
    Inode* n = get(index);
    if (n == nullptr)
    {
      std::cout << ">>> VFS::init_directory() bogus inode (nullptr)" << std::endl;
      return -ENOENT;
    }
    
    // note: must be initialized in-order as seen in struct for speed
    n->id   = index;
    n->type = Inode::DIR;
    n->hardlinks = 0;
    n->dsize = n->indsize = 0;
    
    for (int i = 0; i < Inode::POINTERS; i++)
      n->data[i] = UNUSED_SEG;
    n->name = name;
    
    // mark node as used
    node_bitmap.set(index);
    
    // verify integrity
    if (node_bitmap.get(index) == false)
    {
      std::cout << ">>> VFS::init_directory() integrity error: node was free" << std::endl;
      return -EIO;
    }
    
    return 0;
  }
  
  int VFS::directory_add(inode_t node, inode_t child)
  {
    Inode* n = get(node);
    if (n == nullptr)
    {
      std::cout << ">>> VFS::directory_add() bogus inode (nullptr)" << std::endl;
      return -ENOENT;
    }
    
    if (!n->isDir())
    {
      std::cout << ">>> VFS::directory_add() not a directory" << std::endl;
      return -ENOTDIR;
    }
    
    // 
    for (int i = 0; i < Inode::POINTERS; i++)
    {
      // FIXME: the last data pointer should be an indirection
      // if the data slot is unused, allocate a new data segment
      if (n->data[i] == UNUSED_SEG)
      {
        // create data segment
        int seg = create_segment();
        if (!validate_seg(seg)) return seg;
        // set and initialize as directory
        n->data[i] = seg;
        new (getseg(seg)) VFSdir();
      }
      // since we are here, a directory must exist
      VFSdir* dir = (VFSdir*) getseg(n->data[i]);
      // find free link
      for (int L = 0; L < VFSdir::LINKS; L++)
      if (dir->nodes[L] == UNUSED_NODE)
      {
        // one more link from this node
        n->hardlinks++;
        // add directory in free link slot
        dir->nodes[L] = child;
        std::cout << ">>> VFS::directory_add() added " << child << " to " << n->id << std::endl;
        return 0;
      }
    } // next data pointer
    
    // we couldn't allocate data for the directory
    std::cout << ">>> VFS::directory_add() no space left on device" << std::endl;
    return -ENOSPC;
  }
  
  int VFS::directory_rem(inode_t node, inode_t child)
  {
    Inode* n = get(node);
    if (n == nullptr)
    {
      std::cout << ">>> VFS::directory_rem() bogus inode (nullptr)" << std::endl;
      return -ENOENT;
    }
    
    if (!n->isDir())
    {
      std::cout << ">>> VFS::directory_rem() not a directory" << std::endl;
      return -ENOTDIR;
    }
    
    std::vector<inode_t> list = directory_list(n);
    if (!list.empty())
    {
      std::cout << ">>> VFS::directory_rem() directory not empty" << std::endl;
      return -ENOTEMPTY;
    }
    
    for (int i = 0; i < Inode::POINTERS; i++)
    if (n->data[i] != UNUSED_SEG)
    {
      VFSdir* dir = (VFSdir*) getseg(n->data[i]);
      // find free link
      for (int L = 0; L < VFSdir::LINKS; L++)
      {
        if (dir->nodes[L] == child)
        {
          // check that the directory is empty
          Inode* rem_node = get(child);
          if (rem_node->hardlinks)
          {
            // directory not empty
            return -ENOTEMPTY;
          }
          
          // one less link from this node
          n->hardlinks++;
          // remove child
          dir->nodes[L] = 0;
          return 0;
        }
      } // next file
    } // next data pointer
    
    // no such file or directory
    return -ENOENT;
  }
  
  int VFS::directory_find(inode_t node, Inode*& result, const std::string& name)
  {
    // retrieve node from node table
    Inode* n = get(node);
    if (unlikely(n == nullptr))
    {
      std::cout << ">>> VFS::directory_find() bogus inode (nullptr)" << std::endl;
      return -ENOENT;
    }
    
    // verify node is directory
    if (unlikely(!n->isDir()))
    {
      std::cout << ">>> VFS::directory_find() not a directory" << std::endl;
      return -ENOTDIR;
    }
    
    // read directory contents, find @name
    std::vector<inode_t> list = directory_list(n);
    std::cout << ">>> VFS::directory_find() list size " << n->id << ": " << list.size() << std::endl;
    for (inode_t child : list)
    {
      Inode* c = get(child);
      if (c)
      {
        std::cout << "Testing " << c->name << " (" << c->id << ") vs " << name << std::endl;
        if (c->name == name)
        {
          result = c;
          return 0;
        }
      }
    }
    std::cout << ">>> VFS::directory_find() not found: " << name << std::endl;
    return -ENOENT;
  }
  
  std::vector<inode_t> VFS::directory_list(Inode* node)
  {
    std::vector<inode_t> result;
    
    for (int i = 0; i < Inode::POINTERS; i++)
    if (node->data[i] != UNUSED_SEG)
    {
      // since we are here, a directory must exist
      VFSdir* dir = (VFSdir*) getseg(node->data[i]);
      // find free link
      for (int L = 0; L < VFSdir::LINKS; L++)
      if (dir->nodes[L] != UNUSED_NODE)
      {
        result.push_back(dir->nodes[L]);
      }
    } // next data pointer
    return result;
  }
  
  int VFS::walk_path(Path path, inode_t& current, std::string& new_name)
  {
    // validate path
    if (unlikely(path.getState() != 0))
      return path.getState();
    
    // path must have at least one in the stack
    if (unlikely(path.size() == 0))
      return -EINVAL;
    
    // save top of stack for later
    new_name = path[path.size()-1];
    // FIXME: new_name cannot be empty
    
    // go one up in the path
    path.up();
    
    // start walking at root
    current = ROOT_ID;
    
    // walk filesystem tree
    for (size_t i = 0; i < path.size(); i++)
    {
      Inode* result = nullptr;
      int res = directory_find(current, result, path[i]);
      std::cout << "find " << path[i] << ": " << fs_error_string(res) << std::endl;
      
      if (res) return res;
      
      if (unlikely(result == nullptr))
      {
        std::cout << ">>> VFS::walk_path() bogus result (nullptr)" << std::endl;
        return -ENOENT;
      }
      
      // result was found, verify that it is a directory
      if (!result->isDir())
      {
        std::cout << ">>> VFS::walk_path() not a directory (" << result->name << ")" << std::endl;
        return -ENOTDIR;
      }
      
      std::cout << "current old: " << current << " new: " << result->id << std::endl;
      current = result->id;
    }
    return 0;
  }
  
}
