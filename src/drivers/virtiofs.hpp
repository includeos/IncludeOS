#pragma once
#ifndef VIRTIO_FILESYSTEM_HPP
#define VIRTIO_FILESYSTEM_HPP

#include <string>
#include <unordered_map>

#include <sys/types.h>
#include <cstdint>
#include <cstring>

#include <hw/vfs_device.hpp>
#include <hw/pci_device.hpp>
#include <modern_virtio/control_plane.hpp>
#include <modern_virtio/split_queue.hpp>
#include <fuse/fuse.hpp>

#define FUSE_MAJOR_VERSION 7
#define FUSE_MINOR_VERSION_MIN 36

typedef struct __attribute__((packed)) virtio_fs_init_req {
  fuse_in_header in_header;
  fuse_init_in init_in;

  virtio_fs_init_req(uint32_t majo, uint32_t mino, uint64_t uniqu, uint64_t nodei)
  : in_header(sizeof(fuse_init_in), FUSE_INIT, uniqu, nodei),
    init_in(majo, mino) {}
} virtio_fs_init_req;

typedef struct __attribute__((packed)) {
  fuse_out_header out_header;
  fuse_init_out init_out; // out.len - sizeof(fuse_out_header)
} virtio_fs_init_res;

typedef struct __attribute__((packed)) virtio_fs_lookup_req {
  fuse_in_header in_header;

  virtio_fs_lookup_req(uint32_t plen, uint64_t uniqu, uint64_t nodei) 
  : in_header(plen, FUSE_LOOKUP, uniqu, nodei) {}
} virtio_fs_lookup_req;

typedef struct __attribute__((packed)) {
  fuse_out_header out_header;
  fuse_entry_param entry_param;
} virtio_fs_lookup_res;

typedef struct __attribute__((packed)) virtio_fs_open_req {
  fuse_in_header in_header;
  fuse_open_in open_in;

  virtio_fs_open_req(uint32_t flag, uint32_t open_flag, uint64_t uniqu, uint64_t nodei)
  : in_header(sizeof(fuse_open_in), FUSE_OPEN, uniqu, nodei), 
    open_in(flag, open_flag) {}
} virtio_fs_open_req;

typedef struct __attribute__((packed)) {
  fuse_out_header out_header;
  fuse_open_out open_out;
} virtio_fs_open_res;

typedef struct __attribute__((packed)) virtio_fs_read_req {
  fuse_in_header in_header;
  fuse_read_in read_in;

  virtio_fs_read_req(uint64_t f, uint64_t offse, uint32_t siz, uint64_t uniqu, uint64_t nodei)
  : in_header(sizeof(fuse_read_in), FUSE_READ, uniqu, nodei),
    read_in(f, offse, siz, 0, 0) {} 
} virtio_fs_read_req;

typedef struct __attribute__((packed)) {
  fuse_out_header out_header;
} virtio_fs_read_res;

typedef struct __attribute__((packed)) virtio_fs_write_req {
  fuse_in_header in_header;
  fuse_write_in write_in;

  virtio_fs_write_req(uint64_t f, uint64_t offse, uint32_t siz, uint64_t uniqu, uint64_t nodei) 
  : in_header(sizeof(fuse_write_in) + siz, FUSE_WRITE, uniqu, nodei),
    write_in(f, offse, siz, 0, 0) {}
} virtio_fs_write_req;

typedef struct __attribute__((packed)) virtio_fs_write_res {
  fuse_out_header out_header;
  fuse_write_out write_out;
} virtio_fs_write_res;

typedef struct __attribute__((packed)) virtio_fs_close_req {
  fuse_in_header in_header;
  fuse_release_in release_in;

  virtio_fs_close_req(uint64_t f, uint32_t flag, uint32_t release_flag, uint64_t uniqu, uint64_t nodei)
  : in_header(sizeof(fuse_release_in), FUSE_RELEASE, uniqu, nodei), 
    release_in(f, flag, release_flag) {}
} virtio_fs_close_req;

typedef struct __attribute__((packed)) {
  fuse_out_header out_header;
} virtio_fs_close_res;

/* Virtio configuration stuff */
#define REQUIRED_VFS_FEATS 0ULL

typedef struct {
  fuse_ino_t ino;
  off_t offset;
} fh_info;

class VirtioFS_device : 
  public Virtio_control, 
  public hw::VFS_device
{
public:
  /** Constructor and VirtioFS driver factory */
  VirtioFS_device(hw::PCI_Device& d);

  void deactivate() override;
  void flush() override;

  static std::unique_ptr<hw::VFS_device> new_instance(hw::PCI_Device& d);

  int id() const noexcept override;

  /** Overriden device base functions */
  std::string device_name() const override;

  /** VFS operations overriden with mock functions for now */
  uint64_t open(char *pathname, uint32_t flags, mode_t mode) override;
  off_t lseek(uint64_t fh, off_t offset, int whence) override;
  ssize_t write(uint64_t fh, void *buf, uint32_t count) override;
  ssize_t read(uint64_t fh, void *buf, uint32_t count)  override;
  int close(uint64_t fh) override;

private:
  Split_queue _req;
  std::unordered_map<uint64_t, fh_info> _fh_info_map;
  uint64_t _unique_counter;
  int _id;
};

#endif