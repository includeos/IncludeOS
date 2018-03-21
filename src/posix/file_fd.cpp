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

int File_FD::read(void* p, size_t n) {
  auto buf = ent_.read(offset_, n);
  memcpy(p, buf.data(), std::min(n, buf.size()));
  offset_ += buf.size();
  return buf.size();
}

int File_FD::write(const void*, size_t) {
  return -1;
}

int File_FD::close() {
  return 0;
}

int File_FD::lseek(off_t offset, int whence)
{
  if ((whence != SEEK_SET) && (whence != SEEK_CUR) && (whence != SEEK_END)) {
    //PRINT("lseek(%lu, %d) == %d\n", offset, whence, -1);
    errno = EINVAL;
    return -1;
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


//struct linux_dirent {
// unsigned long  d_ino;     /* Inode number */
// unsigned long  d_off;     /* Offset to next linux_dirent */
// unsigned short d_reclen;  /* Length of this linux_dirent */
// char           d_name[];  /* Filename (null-terminated) */
//                   /* length is actually (d_reclen - 2 -
//                      offsetof(struct linux_dirent, d_name)) */
// /*
// char           pad;       // Zero padding byte
// char           d_type;    // File type (only since Linux
//                           // 2.6.4); offset is (d_reclen - 1)
// */
//}



/*
       DT_BLK      This is a block device.

       DT_CHR      This is a character device.

       DT_DIR      This is a directory.

       DT_FIFO     This is a named pipe (FIFO).

       DT_LNK      This is a symbolic link.

       DT_REG      This is a regular file.

       DT_SOCK     This is a UNIX domain socket.
*/
/*struct dirent
{
  ino_t d_ino;
  off_t d_off;
  unsigned short d_reclen;
  unsigned char d_type;
  char d_name[256];
};*/
struct __dirstream
{
  int fd;
  off_t tell;
  int buf_pos;
  int buf_end;
  //volatile int lock[1];
  char buf[2048];
};
long File_FD::getdents(struct dirent *dirp, unsigned int count)
{
  static_assert(NAME_MAX+1 == sizeof(dirent::d_name));
  printf("getdents(%p, %u) fd=%i \"%s\" (%s)\n", dirp, count, get_id(), ent_.name().c_str(), ent_.type_string().c_str());

  auto* dir = (__dirstream*)(((char*)dirp) - sizeof(__dirstream) + sizeof(__dirstream::buf));
  //printf("dirp %p dir %p buf %p\n", dirp, dir, dir->buf);
  //assert((void*)dir->buf == (void*)dirp);

  if(not ent_.is_dir())
    return -ENOTDIR;

  unsigned int written = 0;
  auto list = ent_.ls();

  auto* ptr = (char*)dirp;
  //ptr -= 4;
  for(auto& ent : list)
  {
    dirp = (dirent*) ptr;
    //if(ent.name() == "." or ent.name() == "..")
    //  continue;
    // inode number
    dirp->d_ino = ent.block();

    // file type
    switch(ent.type())
    {
      case fs::DIR:
        dirp->d_type = DT_DIR;
        break;
      default:
        dirp->d_type = DT_REG;
    }
    // offset to next dirent
    dirp->d_off = 0;

    // filename
    auto name = ent.name();
    const auto namelen = std::min(sizeof(dirp->d_name)-1, name.length());

    // Length of this dirent
    dirp->d_reclen = sizeof(dirent) - sizeof(dirp->d_name) + namelen + 1;

    written += dirp->d_reclen;

    if(written > count)
      return -EINVAL;

    //std::memcpy(dirp->d_name, name.data(), namelen);
    //dirp->d_name[namelen] = '\0';
    std::strncpy(dirp->d_name, name.data(), namelen);

    ptr += dirp->d_reclen;
  }
  //printf("dir %p fd=%u sizeof(buf)=%zu %p\n", dir, dir->fd, sizeof(dir->buf), dir->buf);

  dir->buf_end = written;
  dir->buf_pos = 0;

  while(dir->buf_pos < dir->buf_end)
  {
    auto* de = (struct dirent*)(dir->buf + dir->buf_pos);
    printf("ino: %zu name: %s type: %u len: %u\n", de->d_ino, de->d_name, de->d_type, de->d_reclen);
    if(de->d_reclen == 0) break;
    dir->buf_pos += de->d_reclen;
    dir->tell = de->d_off;
  }

  return written;
}
