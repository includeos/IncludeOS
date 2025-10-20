#pragma once
#include <fs/filesystem.hpp>

namespace fs
{
  class MockFS : public File_system
  {
    int device_id() const noexcept override {
      return 0;
    }

    void ls(const std::string& path, on_ls_func) const override;
    void ls(const Dirent& entry,     on_ls_func) const override;
    List ls(const std::string& path) const override;
    List ls(const Dirent&) const override;

    void   read(const Dirent&, uint64_t pos, uint64_t n, on_read_func) const override;
    Buffer read(const Dirent&, uint64_t pos, uint64_t n) const override;

    void   stat(Path_ptr, on_stat_func fn, const Dirent* const = nullptr) const override;
    Dirent stat(Path, const Dirent* const = nullptr) const override;
    void   cstat(const std::string& pathstr, on_stat_func) override;

    std::string name() const override {
      return "Mock FS";
    }

    uint64_t block_size() const override {
      return 512;
    }

    void init(uint64_t lba, uint64_t size, on_init_func on_init) override;
  };
}
