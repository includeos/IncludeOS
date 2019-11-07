
#ifndef FILE_FD_HPP
#define FILE_FD_HPP

#include "fd.hpp"
#include <fs/dirent.hpp>

class File_FD : public FD {
public:
  explicit File_FD(const int id, const fs::Dirent ent, uint64_t offset = 0)
      : FD(id), ent_ {ent}, offset_ {offset},
        dir_pos{0}, dir_{nullptr}
  {}

  ssize_t read(void*, size_t) override;
  ssize_t readv(const struct iovec*, int iovcnt) override;
  int write(const void*, size_t) override;
  int close() override;
  off_t lseek(off_t, int) override;

  long getdents(struct dirent *dirp, unsigned int count) override;

  bool is_file() override { return true; }

private:
  fs::Dirent ent_;
  uint64_t offset_;
  size_t dir_pos;
  fs::Dirvec_ptr dir_;
};

#endif
