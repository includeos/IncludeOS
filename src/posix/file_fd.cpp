// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
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

#include <posix/file_fd.hpp>
#include <errno.h>
#include <dirent.h>
#include <sys/uio.h>

ssize_t File_FD::read(void* p, size_t n)
{
  Expects(p != nullptr);

  if(UNLIKELY(ent_.is_dir()))
    return -EISDIR;

  if(UNLIKELY(n == 0))
    return 0;

  auto buf = ent_.read(offset_, n);
  if (not buf.is_valid()) return -EIO;

  memcpy(p, buf.data(), std::min(n, buf.size()));
  offset_ += buf.size();
  return buf.size();
}

ssize_t File_FD::readv(const struct iovec* iov, int iovcnt)
{
  if(UNLIKELY(iovcnt <= 0 and iovcnt > IOV_MAX))
    return -EINVAL;

  ssize_t total = 0;
  // TODO: return EINVAL if total overflow ssize_t
  for(int i = 0; i < iovcnt; i++)
  {
    auto& vec = iov[i];
    if(vec.iov_len > 0)
      total += this->read(vec.iov_base, vec.iov_len);
  }

  return total;
}

int File_FD::write(const void*, size_t) {
  return -1;
}

int File_FD::close() {
  return 0;
}

off_t File_FD::lseek(off_t offset, int whence)
{
  if ((whence != SEEK_SET) && (whence != SEEK_CUR) && (whence != SEEK_END)) {
    //PRINT("lseek(%lu, %d) == %d\n", offset, whence, -1);
    return -EINVAL;
  }
  off_t calculated_offset = offset_;
  if (whence == SEEK_SET) {
    calculated_offset = offset;
  }
  else if (whence == SEEK_CUR) {
    calculated_offset += offset;
  }
  else if (whence == SEEK_END) {
    const off_t end = ent_.size();
    calculated_offset = end + offset;
  }
  if (calculated_offset < 0) calculated_offset = 0;
  offset_ = calculated_offset;
  //PRINT("lseek(%lu, %d) == %d\n", offset, whence, offset_);
  return offset_;
}

long File_FD::getdents(struct dirent *dirp, unsigned int count)
{
  static_assert(sizeof(dirent::d_name) > 0);

  if(not ent_.is_dir())
    return -ENOTDIR;

  // open (ls) directory if it havent yet
  if(dir_ == nullptr)
  {
    auto list = ent_.ls();
    if(UNLIKELY(list.error))
      return -ENOENT;

    dir_ = std::move(list.entries);
    dir_pos = 0;
  }

  // we've read every entry
  if(dir_pos == dir_->size())
    return 0;

  auto* ptr = reinterpret_cast<char*>(dirp);
  unsigned int written = 0;

  while(dir_pos != dir_->size())
  {
    // retreive the next entry from the read directory
    const auto& ent = dir_->at(dir_pos);

    // move to next dirent ptr
    dirp = reinterpret_cast<dirent*>(ptr);

    // inode number
    dirp->d_ino = ent.block();
    // offset to next dirent (we dont care, i think..)
    dirp->d_off = 0;

    // file type
    switch(ent.type())
    {
      case fs::DIR:
        dirp->d_type = DT_DIR;
        break;
      default:
        dirp->d_type = DT_REG;
    }

    // filename (make sure it fits in buffer)
    auto name = ent.name();
    const auto namelen = std::min(sizeof(dirp->d_name)-1, name.length());

    // length of this dirent (struct + name)
    const auto len = sizeof(dirent) - sizeof(dirp->d_name) + namelen + 1;

    // if this entry do not fit in the buffer, break
    if((written + len) > count)
      break;

    // set the length
    dirp->d_reclen = len;

    // increase how much we've written
    written += dirp->d_reclen;

    // add the filename
    std::strncpy(dirp->d_name, name.data(), namelen);

    // iterate to next
    ptr += dirp->d_reclen;
    dir_pos++;
  }

  // we haven't written a single entry, which means it didnt fit
  if(UNLIKELY(written == 0))
    return -EINVAL;

  return written;
}
