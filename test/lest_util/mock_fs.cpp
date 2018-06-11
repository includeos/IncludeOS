#include "mock_fs.hpp"

namespace fs
{
  void MockFS::ls(const std::string& path, on_ls_func) const
  {

  }
  void MockFS::ls(const Dirent& entry,     on_ls_func) const
  {

  }
  List MockFS::ls(const std::string& path) const
  {

  }
  List MockFS::ls(const Dirent&) const
  {

  }

  void   MockFS::read(const Dirent&, uint64_t pos, uint64_t n, on_read_func) const
  {

  }
  Buffer MockFS::read(const Dirent&, uint64_t pos, uint64_t n) const
  {

  }
  void   MockFS::stat(Path_ptr, on_stat_func fn, const Dirent* const ent) const
  {

  }
  Dirent MockFS::stat(Path, const Dirent* const ent) const
  {

  }
  void   MockFS::cstat(const std::string& pathstr, on_stat_func)
  {

  }
  void MockFS::init(uint64_t lba, uint64_t size, on_init_func on_init)
  {

  }

}
