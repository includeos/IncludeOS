
// This file contains a plugin for automatically mounting
// resources to the virtual filesystem (VFS)

#include <fs/vfs.hpp>
#include <posix/fd_map.hpp>

#ifndef RAPIDJSON_HAS_STDSTRING
  #define RAPIDJSON_HAS_STDSTRING 1
#endif

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <memdisk>
static hw::Block_device& get_block_device(const std::string& str)
{
  if(str == "memdisk")
  {
    auto& disk = fs::memdisk();
    return disk.dev();
  }

  throw std::runtime_error{"Unsupported disk type"};
}

#include <config>
static void parse_config()
{
  const auto& json = ::Config::get();

  // Nothing to do with empty config
  if(json.empty())
    return;

  using namespace rapidjson;
  Document doc;
  doc.Parse(json.data());

  Expects(doc.IsObject() && "Malformed config");

  // No vfs member, ignore
  if(not doc.HasMember("vfs"))
    return;

  const auto& cfg = doc["vfs"];

  Expects(cfg.IsArray() && "Member vfs is not an array");

  auto mounts = cfg.GetArray();
  if(mounts.Size() == 0)
  {
    INFO("VFS Mounter", "NOTE: No VFS entries - nothing to do");
    return;
  }

  INFO("VFS Mounter", "Found %u VFS entries", mounts.Size());

  for(auto& val : mounts)
  {
    Expects(val.HasMember("disk") && "Missing disk");
    Expects(val.HasMember("root") && "Missing root");
    Expects(val.HasMember("mount") && "Missing mount");
    Expects(val.HasMember("description") && "Missing description");

    auto& dev = get_block_device(val["disk"].GetString());
    auto disk = fs::VFS::insert_disk(dev);

    // TODO: maybe move this code to get_block_device
    if (not disk->fs_ready())
    {
      disk->init_fs([](fs::error_t err, auto&) {
        Expects(not err && "Error mounting disk");
      });
    }

    Expects(disk->fs_ready() && "Filesystem not ready (async disk?)");

    fs::VFS::mount(
      fs::Path{val["mount"].GetString()},
      dev.id(),
      fs::Path{val["root"].GetString()},
      val["description"].GetString(),
      [](auto err) {
        Expects(not err && "Error occured while mounting");
      });
  }

}

#include <posix/rng_fd.hpp>
// Mount RNG functionality to system paths
static void mount_rng()
{
  RNG::get().open_fd = []()->FD& {
    return FD_map::_open<RNG_fd>();
  };

  fs::mount("/dev/random", RNG::get(), "Random device");
  fs::mount("/dev/urandom", RNG::get(), "Random device");
}

// Function being run by the OS for mounting resources
static void vfs_mount()
{
  // RNG
  mount_rng();

  parse_config();
}

#include <os>
__attribute__((constructor))
static void vfs_mount_plugin() {
  os::register_plugin(vfs_mount, "VFS Mounter");
}
