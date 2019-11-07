
#include <common.cxx>
#include <memdisk>

CASE("memdisk properties")
{
  fs::MemDisk memdisk{0,0};
  fs::Disk disk{memdisk};
  EXPECT(disk.empty() == true);
  EXPECT(disk.device_id() == 0);
  EXPECT(disk.fs_ready() == false);
  EXPECT(disk.name() == "memdisk0");
  EXPECT(disk.dev().size() == 0ull);
  EXPECT(disk.dev().device_type() == hw::Device::Type::Block);
  EXPECT(disk.dev().driver_name() == std::string("MemDisk"));
  bool enumerated_partitions {false};

  disk.partitions(
    [&enumerated_partitions, &lest_env]
    (auto err, auto& partitions)
    {
      EXPECT(err);
      enumerated_partitions = true;
      EXPECT(partitions.size() == 0u); // 4 is default number
    });
  EXPECT(enumerated_partitions == true);
}
