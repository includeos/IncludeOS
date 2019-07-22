
#include <fs/dirent.hpp>

namespace fs
{
  Dirent::Dirent(const Dirent& other) noexcept
  {
    this->fs_     = other.fs_;
    this->ftype   = other.ftype;
    this->fname_  = other.fname_;
    this->block_  = other.block_;
    this->parent_ = other.parent_;
    this->size_   = other.size_;
    this->attrib_ = other.attrib_;
    this->modif   = other.modif;
  }
}
