#include <common.cxx>
#include <fs/disk.hpp>
#include <fs/memdisk.hpp>

using namespace fs;

CASE("Initialize mock FS")
{
  fs::MemDisk memdisk {0, 0};
  fs::Disk disk { memdisk };
  
  EXPECT(disk.empty());
  EXPECT(disk.device_id() >= 0);
  EXPECT(disk.name().size() > 1); // name0
  disk.init_fs(
    [&] (fs::error_t error, fs::File_system& fs)
    {
      EXPECT(error != fs::no_error);
    });
  
}
