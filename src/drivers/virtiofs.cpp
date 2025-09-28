#include "virtiofs.hpp"

#include <memory>
#include <sys/types.h>
#include <string>

#include <hw/pci_manager.hpp>
#include <info>

VirtioFS_device::VirtioFS_device(hw::PCI_Device& d) : 
Virtio_control(d), _req(*this, 1, true), _unique_counter(0)
{
  static int id_count = 0;
  _id = id_count++;
  set_driver_ok_bit();

  /* Negotiate FUSE version */
  virtio_fs_init_req init_req(FUSE_MAJOR_VERSION, FUSE_MINOR_VERSION_MIN, _unique_counter++, FUSE_ROOT_ID);
  virtio_fs_init_res init_res {};

  VirtTokens init_req_tokens;
  init_req_tokens.reserve(2);
  init_req_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS, 
    reinterpret_cast<uint8_t*>(&init_req), 
    sizeof(virtio_fs_init_req)
  );
  init_req_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE, 
    reinterpret_cast<uint8_t*>(&init_res), 
    sizeof(virtio_fs_init_res)
  );

  _req.enqueue(init_req_tokens);
  _req.kick();

  while(_req.has_processed_used());
  _req.dequeue();

  bool compatible_major_version = (FUSE_MAJOR_VERSION == init_res.init_out.major);
  CHECK(compatible_major_version, "Daemon and driver major FUSE version matches");
  Expects(compatible_major_version);

  bool compatible_minor_version = (FUSE_MINOR_VERSION_MIN <= init_res.init_out.minor);
  CHECK(compatible_minor_version, "Daemon falls back to the driver supported minor FUSE version");
  Expects(compatible_minor_version);

  /* Finalizing initialization */
  INFO("VirtioFS", "Device initialization is now complete");
}

void VirtioFS_device::deactivate() {
  flush();
  deactivate_virtio_control();
}

void VirtioFS_device::flush() {}

/** Factory method used to create VirtioFS driver object */
std::unique_ptr<hw::VFS_device> VirtioFS_device::new_instance(hw::PCI_Device& d) {
  return std::make_unique<VirtioFS_device>(d);
}

int VirtioFS_device::id() const noexcept {
  return _id;
}

/** Method returns the name of the device */
std::string VirtioFS_device::device_name() const {
  return "VirtioFS" + std::to_string(_id);
}

uint64_t VirtioFS_device::open(char *pathname, uint32_t flags, mode_t /*mode*/) {
  /* FUSE lookup */
  uint32_t pathname_len = strlen(pathname) + 1;

  virtio_fs_lookup_req lookup_req(pathname_len, _unique_counter++, FUSE_ROOT_ID);
  virtio_fs_lookup_res lookup_res {};

  VirtTokens lookup_tokens;
  lookup_tokens.reserve(3);
  lookup_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    reinterpret_cast<uint8_t*>(&lookup_req),
    sizeof(virtio_fs_lookup_req)
  );
  lookup_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    reinterpret_cast<uint8_t*>(pathname),
    pathname_len
  );
  lookup_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE,
    reinterpret_cast<uint8_t*>(&lookup_res),
    sizeof(virtio_fs_lookup_res)
  );

  _req.enqueue(lookup_tokens);
  _req.kick();

  while(_req.has_processed_used());
  uint32_t device_written_len;
  _req.dequeue(&device_written_len);

  if (lookup_res.out_header.error != 0) {
    return -1;
  }

  fuse_ino_t ino = lookup_res.entry_param.ino;

  /* Creating a file handle */
  virtio_fs_open_req open_req(flags, 0, _unique_counter++, ino);
  virtio_fs_open_res open_res {};

  VirtTokens open_tokens;
  open_tokens.reserve(2);
  open_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    reinterpret_cast<uint8_t*>(&open_req),
    sizeof(virtio_fs_open_req)
  );
  open_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE,
    reinterpret_cast<uint8_t*>(&open_res),
    sizeof(virtio_fs_open_res)
  );

  _req.enqueue(open_tokens);
  _req.kick();

  while(_req.has_processed_used());
  _req.dequeue();

  if (open_res.out_header.error != 0) {
    return -1;
  }

  /* Inserting into fh_ino mapping */
  uint64_t fh = open_res.open_out.fh;
  
  _fh_info_map[fh] = {ino, 0};

  return fh;
}

off_t VirtioFS_device::lseek(uint64_t fh, off_t offset, int whence) {
  if (not _fh_info_map.contains(fh)) return -1;
  
  // TODO: Find ways to avoid integer overflows
  // TODO: Figure out how to do shit POSIX stuff with errno
  off_t new_offset;
  switch(whence) {
    case SEEK_SET:
      new_offset = offset;
      break;
    case SEEK_CUR:
      new_offset = _fh_info_map[fh].offset + offset;
      break;
    default:
      return -1;
  }

  _fh_info_map[fh].offset = new_offset;
  return new_offset;
}

ssize_t VirtioFS_device::write(uint64_t fh, void *buf, uint32_t count) {
  if (not _fh_info_map.contains(fh)) return -1;

  fuse_ino_t ino = _fh_info_map[fh].ino;
  off_t offset = _fh_info_map[fh].offset;

  /* FUSE write request */
  virtio_fs_write_req write_req(fh, offset, count, _unique_counter++, ino);
  virtio_fs_write_res write_res{};

  VirtTokens write_tokens;
  write_tokens.reserve(3);

  write_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    reinterpret_cast<uint8_t*>(&write_req),
    sizeof(virtio_fs_write_req)
  );
  write_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    reinterpret_cast<uint8_t*>(buf),
    count
  );
  write_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE,
    reinterpret_cast<uint8_t*>(&write_res),
    sizeof(virtio_fs_write_res)
  );

  _req.enqueue(write_tokens);
  _req.kick();

  while(_req.has_processed_used());
  _req.dequeue();

  if (write_res.out_header.error != 0) return -1;

  /* Updating seek offset and returning */
  ssize_t write_count = write_res.write_out.size;
  _fh_info_map[fh].offset += write_count;

  return write_count;
}

ssize_t VirtioFS_device::read(uint64_t fh, void *buf, uint32_t count) {
  if (not _fh_info_map.contains(fh)) return -1;

  fuse_ino_t ino = _fh_info_map[fh].ino;
  off_t offset = _fh_info_map[fh].offset;

  /* FUSE read request */
  virtio_fs_read_req read_req(fh, offset, count, _unique_counter++, ino);
  virtio_fs_read_res read_res{};

  VirtTokens read_tokens;
  read_tokens.reserve(3);

  read_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS, 
    reinterpret_cast<uint8_t*>(&read_req),
    sizeof(virtio_fs_read_req)
  );
  read_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE, 
    reinterpret_cast<uint8_t*>(&read_res),
    sizeof(virtio_fs_read_res)
  );
  read_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE, 
    reinterpret_cast<uint8_t*>(buf),
    count
  );

  _req.enqueue(read_tokens);
  _req.kick();

  while(_req.has_processed_used());
  _req.dequeue();

  if (read_res.out_header.error != 0) return -1;

  /* Updating seek offset and returning */
  ssize_t read_count = read_res.out_header.len - sizeof(fuse_out_header);
  _fh_info_map[fh].offset += read_count;

  return read_count;
}

int VirtioFS_device::close(uint64_t fh) {
  if (not _fh_info_map.contains(fh)) return -1;
  fuse_ino_t ino = _fh_info_map[fh].ino;
  _fh_info_map.erase(fh);

  /* FUSE close request */
  virtio_fs_close_req close_req(fh, 0, 0, _unique_counter++, ino); 
  virtio_fs_close_res close_res{};

  VirtTokens close_tokens;
  close_tokens.reserve(2);
  close_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    reinterpret_cast<uint8_t*>(&close_req),
    sizeof(virtio_fs_close_req)
  );
  close_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE,
    reinterpret_cast<uint8_t*>(&close_res),
    sizeof(virtio_fs_close_res)
  );

  _req.enqueue(close_tokens);
  _req.kick();

  while(_req.has_processed_used());
  _req.dequeue();

  if (close_res.out_header.error != 0) {
    return -1;
  }

  return 0;
}

__attribute__((constructor))
void autoreg_virtiofs() {
  // Make this part less hacky for the future
  hw::PCI_manager::register_vfs(PCI::VENDOR_VIRTIO, 0x105a, &VirtioFS_device::new_instance);
}
